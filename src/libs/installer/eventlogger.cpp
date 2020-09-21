#include "eventlogger.h"

#include <QByteArray>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSettings>
#include <QSslConfiguration>
#include <QString>
#include <QTimer>
#include <QFuture>
#include <QThread>
#include <qtconcurrentrun.h>
#include <QUuid>

Q_LOGGING_CATEGORY(lcEventLogger, "events")

EventLogger::EventLogger()
    : m_manager(nullptr)
{
    QCryptographicHash hasher(QCryptographicHash::Md5);

    auto interfaces = QNetworkInterface::allInterfaces();
    if(!interfaces.isEmpty())
    {
        auto macAddress = interfaces.first().hardwareAddress();
        hasher.addData(macAddress.toLocal8Bit());
    }

    QString timestamp = QString::fromLatin1("%1").arg(QDateTime::currentMSecsSinceEpoch());
    hasher.addData(timestamp.toLocal8Bit());
    QString randomNumber = QString::fromLatin1("%1").arg(QRandomGenerator::securelySeeded().generate());
    hasher.addData(randomNumber.toLocal8Bit());

    m_sessionId = hasher.result();

    /*QSettings::Format format = (windowsBitness() == 2) ? QSettings::Registry64Format : QSettings::Registry32Format;
    QSettings winVersion(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography"), format);
    if(winVersion.contains(QLatin1String("MachineGuid")))
    {
        QUuid osUuid = QUuid::fromString(winVersion.value(QLatin1String("MachineGuid")).toString());
        m_operatingSystemUuid = osUuid.toRfc4122();
    }*/

    //m_session = QString::fromLatin1("ls") + hasher.result().toHex(); //causes error: error C2664: 'void QConcatenable<QByteArray>::appendTo(const QByteArray &,char *&)': cannot convert argument 2 from 'T *' to 'char *&'

}

void EventLogger::Initialize(bool isChina, const QString& version)
{
    s_isChina = isChina;
    s_version = version;
}

EventLogger *EventLogger::instance()
{
    static EventLogger s_instance;
    return &s_instance;
}

QString EventLogger::getSession()
{
    return m_session;
}

//-----------------------------------------MESSAGE PUBLISHING:------------------------------------------------
void EventLogger::publish(const QJsonObject& installerEvent)
{
    QJsonDocument doc(installerEvent);

    //QNetworkAccessManager* nam = new QNetworkAccessManager();
    //QByteArray payload(payloadData.toStdString().c_str());
    QEventLoop synchronous;
    QNetworkRequest request;
/*
    connect(nam, SIGNAL(finished(QNetworkReply*)), &synchronous, SLOT(quit()));
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));

    QNetworkReply* reply = nam->post(request, payload);

    connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));

    synchronous.exec();*/
}


void EventLogger::publishStarted()
{
    QJsonObject started;

    started[QLatin1String("asdf")] = QLatin1String("asdf");

    /*host->set_allocated_os(getOsInfo());
    auto interfaces = QNetworkInterface::allInterfaces();
    if(!interfaces.isEmpty())
    {
        auto address = interfaces.first().hardwareAddress();
        auto macAddress = ParseHardwareAddress(address);
        host->set_allocated_mac_address(new std::string(macAddress.data(), size_t(macAddress.size())));
    }

    payload->set_allocated_host(host);

    queueAllocatedEvent(payload);*/

    publish(started);
}

void EventLogger::publishIntroductionPageOpened()
{
}

void EventLogger::publishLauncherInitializationStarted()
{
}

void EventLogger::publishLauncherInitializationDone()
{
}

void EventLogger::publishTargetDirectoryPageOpened()
{
}

void EventLogger::publishComponentSelectionPageOpened()
{
}

void EventLogger::publishStartMenuPageOpened()
{
}

void EventLogger::publishReadyPageOpened()
{
}

void EventLogger::publishExecutingPageOpened()
{
}

void EventLogger::publishLauncherInstallationStarting()
{
}

void EventLogger::publishLauncherInstalled()
{
}

void EventLogger::publishLauncherInstallationFailed()
{
}

void EventLogger::publishRedistInstallationStarting()
{
}

void EventLogger::publishRedistInstalled()
{
}

void EventLogger::publishRedistInstallationFailed()
{
}

void EventLogger::publishFinishedPageOpened()
{
}
//-----------------------------------------MESSAGE PARTS: ------------------------------------------
/*google::protobuf::Timestamp* EventLogger::getTimestamp(qint64 millisecondsSinceEpoch)
{
    if(millisecondsSinceEpoch == 0)
    {
        millisecondsSinceEpoch = QDateTime::currentMSecsSinceEpoch();
    }

    auto now = new google::protobuf::Timestamp;
    now->set_seconds(millisecondsSinceEpoch / 1000);
    now->set_nanos(static_cast<google::protobuf::int32>(millisecondsSinceEpoch % 1000) * 1000000);
    return now;
}

eve_launcher::Application* EventLogger::getApplication() {
    auto app = new eve_launcher::Application;
    app->set_version(s_version.toStdString());

    auto buildTag = eve_launcher::Application_BuildTag::Application_BuildTag_RELEASE;
    app->set_build_tag(buildTag);

    if(s_isChina)
    {
        app->set_locale(eve_launcher::Application_Locale_CHINA);
    }
    else
    {
        app->set_locale(eve_launcher::Application_Locale_WORLD);
    }

    return app;
}

eve_launcher::OS* EventLogger::getOsInfo(){
    auto os = new eve_launcher::OS;
    os->set_kind(eve_launcher::OS_Kind::OS_Kind_WINDOWS);
    os->set_version(QSysInfo::productVersion().toStdString());
    os->set_buildversion(QSysInfo::kernelVersion().toStdString());
    os->set_prettyproductname(QSysInfo::prettyProductName().toStdString());
    os->set_processorarchitecture(QSysInfo::currentCpuArchitecture().toStdString());
    os->set_bitness(static_cast<eve_launcher::Bitness>(windowsBitness()));

    return os;
}

eve_launcher::IPAddress* EventLogger::toIPAddress(QHostAddress address)
{
    eve_launcher::IPAddress* ipAddress = new eve_launcher::IPAddress;
    if (address.protocol() == QAbstractSocket::IPv4Protocol)
    {
        ipAddress->set_v4(static_cast<google::protobuf::uint32>(address.toIPv4Address()));
    }
    else if (address.protocol() == QAbstractSocket::IPv6Protocol)
    {
        QString ipv6 = address.toString();
        ipAddress->set_v6(ipv6.toStdString());
    }

    return ipAddress;
}
*/

unsigned char hexDigitValue(QChar input)
{
    unsigned char result;
    if(input.isDigit())
    {
        result = static_cast<unsigned char>(input.digitValue());
    }
    else
    {
        result = static_cast<unsigned char>(input.toLatin1()) - static_cast<unsigned char>('a') + 10;
    }
    return result;
}

QByteArray EventLogger::ParseHardwareAddress(QString address)
{
    QByteArray result;

    auto stripped = address.remove(QLatin1String(":")).remove(QLatin1String("-")).remove(QLatin1String(".")).toLower();

    if(stripped.size() % 2 != 0)
    {
        return result;
    }

    for(int i = 0; i < stripped.size(); i += 2)
    {
        unsigned char high = hexDigitValue(stripped[i]);
        unsigned char low = hexDigitValue(stripped[i+1]);
        unsigned char byte = high << 4 | low;
        result.push_back(static_cast<char>(byte));
    }

    return result;
}

/*
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

int EventLogger::windowsBitness()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process && !fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
    {
        return 0;
    }

    return bIsWow64 ? 2 : 1;
}
*/
