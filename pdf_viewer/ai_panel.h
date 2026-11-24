#pragma once

#if defined(SIOYEK_JKQT_MATHTEXT_SUPPORT) && !defined(SIOYEK_ANDROID)

#include <QDockWidget>
#include <QImage>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QPointer>
#include <vector>

class QLabel;
class QComboBox;
class QLineEdit;
class QNetworkReply;
class QPushButton;
class QProcess;
class QTextBrowser;

class AiPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit AiPanel(QWidget* parent);
    ~AiPanel() override;
    void beginConversation(const QString& source, const QString& excerpt, const QImage& snapshot,
                           bool chooseAgent = false);
    void focusQuestion();

signals:
    void closed();
    void toggleRequested();

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
    void setZoom(int percent);
    void applyTheme();
    void stopRequest();

    QTextBrowser* transcript;
    QLabel* attachment;
    QLabel* status;
    QComboBox* agent;
    QComboBox* theme;
    QLineEdit* question;
    QPushButton* send;
    QPushButton* zoomReset;
    QNetworkAccessManager network;
    QPointer<QNetworkReply> pending;
    QPointer<QProcess> agentProcess;
    QJsonArray conversation;
    std::vector<Message> transcriptMessages;
    QString source;
    QString excerpt;
    QImage snapshot;
    int equationNumber = 0;
    int zoomPercent = 100;
};

#endif
