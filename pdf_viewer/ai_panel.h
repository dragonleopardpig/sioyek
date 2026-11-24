#pragma once

#if defined(SIOYEK_JKQT_MATHTEXT_SUPPORT) && !defined(SIOYEK_ANDROID)

#include <QAbstractTextDocumentLayout>
#include <QDockWidget>
#include <QImage>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QPicture>
#include <QPointer>
#include <vector>
#include "ai_document_context.h"
#include "book.h"

class QLabel;
class QComboBox;
class QLineEdit;
class QNetworkReply;
class QPushButton;
class QProcess;
class QTextBrowser;
class QShortcut;
class QThread;
class DatabaseManager;

class AiVectorRenderer : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    struct Graphic {
        QSizeF size;
        QPicture picture;
    };

    using QObject::QObject;
    int add(Graphic graphic);
    void clear();
    QSizeF intrinsicSize(QTextDocument* document, int, const QTextFormat& format) override;
    void drawObject(QPainter* painter, const QRectF& rect, QTextDocument*, int, const QTextFormat& format) override;

private:
    std::vector<Graphic> graphics;
};

class AiPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit AiPanel(DatabaseManager* database, QWidget* parent);
    ~AiPanel() override;
    void beginConversation(const QString& path, const std::string& checksum, const QByteArray& password,
                           int page, const QString& excerpt, const QImage& snapshot,
                           const std::optional<AiNoteAnchor>& anchor, bool chooseAgent = false);
    void setReadingPage(int page);
    bool isShowingChat() const;
    void showSavedNote(const QString& markdown);
    void focusQuestion();

signals:
    void closed();
    void toggleRequested();
    void notesChanged();
    void resumeRequested();

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;

private:
    struct Message {
        QString speaker;
        QString content;
        bool markdown;
    };

    void submit();
    void sendRequest();
    void finishRequest(const QString& answer, const QString& error = {});
    void undoLast();
    void updateRequestControls();
    void refreshChatTranscript(bool scrollToEnd = false);
    bool storeChat(AiChatState& updated, const std::optional<AiNoteUpdate>& note);
    void readPdf();
    void appendMessage(const QString& speaker, const QString& message, bool markdown);
    void insertMessage(const Message& message);
    void rebuildTranscript();
    void exportMarkdown();
    void setZoom(int percent);
    void applyTheme();
    void stopRequest();

    DatabaseManager* database;
    QTextBrowser* transcript;
    AiVectorRenderer* vectorRenderer;
    QLabel* attachment;
    QLabel* status;
    QComboBox* agent;
    QComboBox* theme;
    QLineEdit* question;
    QPushButton* send;
    QPushButton* zoomReset;
    QPushButton* exportButton;
    QPushButton* undoButton;
    QPushButton* resumeButton;
    QShortcut* cancelShortcut;
    QNetworkAccessManager network;
    QPointer<QNetworkReply> pending;
    QPointer<QProcess> agentProcess;
    QPointer<QThread> pdfReader;
    std::optional<AiPdfContext> pdfContext;
    AiChatState chat;
    std::vector<Message> transcriptMessages;
    QString documentPath;
    std::string checksum;
    QByteArray documentPassword;
    int readingPage = -1;
    int attachmentPage = -1;
    QString excerpt;
    QString noteMarkdown;
    QString pendingPrompt;
    QJsonObject submittedTurn;
    QImage snapshot;
    std::optional<AiNoteAnchor> pendingAnchor;
    int graphicNumber = 0;
    int zoomPercent = 100;
};

#endif
