#pragma once

#if defined(SIOYEK_JKQT_MATHTEXT_SUPPORT) && !defined(SIOYEK_ANDROID)

#include <QByteArray>
#include <QStringList>

struct AiPdfContext {
    QStringList pages;
    QString error;
};

AiPdfContext readAiPdf(const QString& path, const QString& checksum, const QByteArray& password);
QString relevantAiPages(const QStringList& pages, int currentPage, const QString& question, qsizetype budget = 18000);

#endif
