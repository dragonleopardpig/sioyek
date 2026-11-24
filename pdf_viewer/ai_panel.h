#pragma once

#if defined(SIOYEK_JKQT_MATHTEXT_SUPPORT) && !defined(SIOYEK_ANDROID)

#include <QDockWidget>
#include <QImage>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QPointer>

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

signals:
    void closed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void submit();
    void finishRequest(const QJsonObject& user, const QString& prompt, const QString& answer, const QString& error);
    void appendMessage(const QString& speaker, const QString& message, bool markdown);
    void stopRequest();

    QTextBrowser* transcript;
    QLabel* attachment;
    QLabel* status;
    QComboBox* agent;
    QLineEdit* question;
    QPushButton* send;
    QNetworkAccessManager network;
    QPointer<QNetworkReply> pending;
    QPointer<QProcess> agentProcess;
    QJsonArray conversation;
    QString source;
    QString excerpt;
    QImage snapshot;
    int equationNumber = 0;
};

#endif
