#ifndef EVENTLOGGER_H
#define EVENTLOGGER_H

#include <QFuture>
#include <QLoggingCategory>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QQueue>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(lcEventLogger)


static bool s_isChina;
static QString s_version;

class EventLogger : QObject
{
    Q_OBJECT

public:
    static EventLogger* instance();
    static void Initialize(bool isChina, const QString& version);

    QString getSession();
    QString getAuthenticationToken() const;
    void setAuthenticationToken(const QString &authenticationToken);
    void waitForFinished();

    void publishStarted();
    void publishIntroductionPageOpened();
    void publishLauncherInitializationStarted();
    void publishLauncherInitializationDone();
    void publishTargetDirectoryPageOpened();
    void publishComponentSelectionPageOpened();
    void publishStartMenuPageOpened();
    void publishReadyPageOpened();
    void publishExecutingPageOpened();
    void publishLauncherInstallationStarting();
    void publishLauncherInstalled();
    void publishLauncherInstallationFailed();
    void publishRedistInstallationStarting();
    void publishRedistInstalled();
    void publishRedistInstallationFailed();
    void publishFinishedPageOpened();



protected:

    void publish(const QJsonObject& installerEvent);

    QNetworkAccessManager* m_manager;
    QString m_session;
    QByteArray m_sessionId;
    QByteArray m_operatingSystemUuid;

    explicit EventLogger();

    /*google::protobuf::Timestamp* getTimestamp(qint64 millisecondsSinceEpoch = 0);
    eve_launcher::Application* getApplication();
    bool sendAllocatedEvent(const EventQueueEntry& payload);
    eve_launcher::user::Identifier* createUserIdentifier(int userId);
    eve_launcher::IPAddress* toIPAddress(QHostAddress address);
    void queueAllocatedEvent(::google::protobuf::Message* payload);
    eve_launcher::OS* getOsInfo();
*/
    //int EventLogger::windowsBitness();
    QByteArray ParseHardwareAddress(QString address);


};

#endif // EVENTLOGGER_H
