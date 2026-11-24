#include "ai_document_context.h"

#if defined(SIOYEK_JKQT_MATHTEXT_SUPPORT) && !defined(SIOYEK_ANDROID)

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>
#include <algorithm>
#include <numeric>
#include <vector>
#include <mupdf/fitz.h>

namespace {

QString readPage(fz_context* context, fz_document* document, int page) {
    fz_buffer* text = nullptr;
    const fz_stext_options options{FZ_STEXT_DEHYPHENATE, 1};
    fz_var(text);
    fz_try(context) {
        text = fz_new_buffer_from_page_number(context, document, page, &options);
    }
    fz_catch(context) {
        return {};
    }
    unsigned char* bytes = nullptr;
    const size_t length = fz_buffer_storage(context, text, &bytes);
    const auto result = QString::fromUtf8(reinterpret_cast<const char*>(bytes), length).trimmed();
    fz_drop_buffer(context, text);
    return result;
}

QStringList readCache(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const auto array = QJsonDocument::fromJson(file.readAll()).array();
    QStringList pages;
    for (const auto page : array)
        pages.append(page.toString());
    return pages;
}

void writeCache(const QString& path, const QStringList& pages) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner))
        return;
    const auto bytes = QJsonDocument(QJsonArray::fromStringList(pages)).toJson(QJsonDocument::Compact);
    if (file.write(bytes) == bytes.size())
        file.commit();
}

}

AiPdfContext readAiPdf(const QString& path, const QString& checksum, const QByteArray& password) {
    AiPdfContext result;
    const QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString directory = QDir(root).filePath(QStringLiteral("ask-ai/pdf-text-v1"));
    const QString cached = QDir(directory).filePath(checksum + QStringLiteral(".json"));
    const bool canCache = password.isEmpty() && !root.isEmpty() && !checksum.isEmpty() && QDir().mkpath(directory);
    QLockFile lock(cached + QStringLiteral(".lock"));
    if (canCache) {
        result.pages = readCache(cached);
        if (!result.pages.isEmpty())
            return result;
        while (!lock.tryLock(100)) {
            if (QThread::currentThread()->isInterruptionRequested())
                return result;
            if (lock.error() != QLockFile::LockFailedError)
                break;
        }
        result.pages = readCache(cached);
        if (!result.pages.isEmpty())
            return result;
    }

    fz_context* context = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!context) {
        result.error = QCoreApplication::translate("AiPanel", "Could not read PDF text.");
        return result;
    }
    fz_document* document = nullptr;
    int count = 0;
    bool encrypted = false;
    const QByteArray filename = path.toUtf8();
    fz_var(document);
    fz_var(count);
    fz_var(encrypted);
    fz_try(context) {
        fz_register_document_handlers(context);
        document = fz_open_document(context, filename.constData());
        encrypted = fz_needs_password(context, document);
        if (!encrypted || fz_authenticate_password(context, document, password.constData()))
            count = fz_count_pages(context, document);
    }
    fz_catch(context) {
        count = 0;
    }
    bool interrupted = false;
    if (count > 0)
        result.pages.reserve(count);
    for (int page = 0; page < count; ++page) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            interrupted = true;
            break;
        }
        result.pages.append(readPage(context, document, page));
    }
    fz_drop_document(context, document);
    fz_drop_context(context);
    if (interrupted)
        return {};

    if (count <= 0) {
        result.error = QCoreApplication::translate("AiPanel", "Could not read PDF text. You can attach a passage or snapshot.");
    } else if (canCache && !encrypted && lock.isLocked()) {
        writeCache(cached, result.pages);
    }
    return result;
}

QString relevantAiPages(const QStringList& pages, int currentPage, const QString& question, qsizetype budget) {
    const qsizetype total = std::accumulate(pages.begin(), pages.end(), qsizetype(0),
        [](qsizetype sum, const QString& page) { return sum + page.size() + 24; });
    std::vector<qsizetype> ranked(pages.size());
    std::iota(ranked.begin(), ranked.end(), qsizetype(0));
    QStringList terms;
    if (total > budget) {
        static const QRegularExpression word(QStringLiteral(R"([\p{L}\p{N}]{2,})"));
        static const QRegularExpression pageReference(QStringLiteral(R"(\bpages?\s+(\d+)\b)"),
                                                       QRegularExpression::CaseInsensitiveOption);
        static const QStringList common{
            "the", "of", "to", "is", "in", "and", "it", "or", "for", "on", "with", "as",
            "an", "be", "can", "what", "how", "does", "this", "that", "here", "you", "explain",
            "derive", "about", "from", "are", "was", "please", "why"
        };
        auto matches = word.globalMatch(question.toLower());
        while (matches.hasNext() && terms.size() < 16) {
            const QString term = matches.next().captured();
            if (!common.contains(term) && !terms.contains(term))
                terms.append(term);
        }
        std::vector<int> scores(pages.size(), 0);
        auto references = pageReference.globalMatch(question);
        while (references.hasNext()) {
            const int page = references.next().captured(1).toInt();
            if (page > 0 && page <= pages.size())
                scores[page - 1] = 200;
        }
        for (qsizetype index = 0; index < pages.size(); ++index) {
            for (const auto& term : terms)
                scores[index] += std::min<qsizetype>(4, pages[index].count(term, Qt::CaseInsensitive));
            if (index == currentPage)
                scores[index] += 100;
            else if (std::abs(index - currentPage) == 1)
                scores[index] += 1;
        }
        std::stable_sort(ranked.begin(), ranked.end(), [&scores](qsizetype left, qsizetype right) {
            return scores[left] > scores[right];
        });
    }

    QString context;
    for (const auto index : ranked) {
        const QString& page = pages[index];
        if (page.isEmpty())
            continue;
        const QString heading = QStringLiteral("\n\n[PDF page %1]\n").arg(index + 1);
        qsizetype available = budget - context.size() - heading.size();
        if (available <= 0)
            break;
        if (total > budget)
            available = std::min<qsizetype>(available, 3500);
        qsizetype start = 0;
        if (page.size() > available && index != currentPage) {
            for (const auto& term : terms) {
                const auto match = page.indexOf(term, 0, Qt::CaseInsensitive);
                if (match >= 0) {
                    start = std::max<qsizetype>(0, match - available / 3);
                    break;
                }
            }
        }
        context += heading + page.mid(start, available);
    }
    return context;
}

#endif
