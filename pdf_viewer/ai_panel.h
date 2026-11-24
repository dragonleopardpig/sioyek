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

class QLabel;
class QComboBox;
class QLineEdit;
class QNetworkReply;
class QPushButton;
class QProcess;
class QTextBrowser;

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
    explicit AiPanel(QWidget* parent);
    ~AiPanel() override;
    void beginConversation(const QString& source, const QString& excerpt, const QImage& snapshot,
                           bool chooseAgent = false);
    void showSavedNote(const QString& markdown);
    void focusQuestion();
    void reportNoteSaveFailure();

signals:
    void closed();
    void toggleRequested();
    void noteMarkdownReady(const QString& markdown);

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
    void finishRequest(const QJsonObject& user, const QString& prompt, const QString& answer, const QString& error);
    void appendMessage(const QString& speaker, const QString& message, bool markdown);
    void insertMessage(const Message& message);
    void rebuildTranscript();
    void exportMarkdown();
    void setZoom(int percent);
    void applyTheme();
    void stopRequest();

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
    QNetworkAccessManager network;
    QPointer<QNetworkReply> pending;
    QPointer<QProcess> agentProcess;
    QJsonArray conversation;
    std::vector<Message> transcriptMessages;
    QString source;
    QString excerpt;
    QString noteMarkdown;
    QImage snapshot;
    int graphicNumber = 0;
    int zoomPercent = 100;
};

#endif
