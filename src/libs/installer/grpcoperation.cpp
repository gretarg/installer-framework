#include "grpcoperation.h"
#include "eventlogger.h"
#include <QMessageBox>
#include <QtCore/QDebug>
#include <QEventLoop>


using namespace QInstaller;

GrpcOperation::GrpcOperation(PackageManagerCore *core)
    : UpdateOperation(core)
{
    setName(QLatin1String("Grpc"));
}

void GrpcOperation::backup()
{
}


bool GrpcOperation::performOperation()
{
    if (!checkArgumentCount(1, 3, tr(" (init|[messagetype], isChina = (true|false) (init only), version (init only))")))
        return false;

    const QStringList args = arguments();
    const QString action = args.at(0);
    const QString payloadData = args.at(1);

    if(action==QLatin1String("init"))
    {
        if (!checkArgumentCount(3, 3, tr(" (init|[messagetype], isChina = (true|false) (init only), version (init only))")))
            return false;

        EventLogger::Initialize(args.at(1) == QLatin1String("true"), args.at(2));
    }
    else if(action == QLatin1String("started"))
    {
        EventLogger::instance()->publishStarted();
    }
    else if(action == QLatin1String("introductionPageOpened"))
    {
        EventLogger::instance()->publishIntroductionPageOpened();
    }
    else if(action == QLatin1String("launcherInitializationStarted"))
    {
        EventLogger::instance()->publishLauncherInitializationStarted();
    }
    else if(action == QLatin1String("launcherInitializationDone"))
    {
        EventLogger::instance()->publishLauncherInitializationDone();
    }
    else if(action == QLatin1String("targetDirectoryPageOpened"))
    {
        EventLogger::instance()->publishTargetDirectoryPageOpened();
    }
    else if(action == QLatin1String("componentSelectionPageOpened"))
    {
        EventLogger::instance()->publishComponentSelectionPageOpened();
    }
    else if(action == QLatin1String("startMenuPageOpened"))
    {
        EventLogger::instance()->publishStartMenuPageOpened();
    }
    else if(action == QLatin1String("readyPageOpened"))
    {
        EventLogger::instance()->publishReadyPageOpened();
    }
    else if(action == QLatin1String("executingPageOpened"))
    {
        EventLogger::instance()->publishExecutingPageOpened();
    }
    else if(action == QLatin1String("launcherInstallationStarting"))
    {
        EventLogger::instance()->publishLauncherInstallationStarting();
    }
    else if(action == QLatin1String("launcherInstalled"))
    {
        EventLogger::instance()->publishLauncherInstalled();
    }
    else if(action == QLatin1String("launcherInstallationFailed"))
    {
        EventLogger::instance()->publishLauncherInstallationFailed();
    }
    else if(action == QLatin1String("redistInstallationStarting"))
    {
        EventLogger::instance()->publishRedistInstallationStarting();
    }
    else if(action == QLatin1String("redistInstalled"))
    {
        EventLogger::instance()->publishRedistInstalled();
    }
    else if(action == QLatin1String("redistInstallationFailed"))
    {
        EventLogger::instance()->publishRedistInstallationFailed();
    }
    else if(action == QLatin1String("finishedPageOpened"))
    {
        EventLogger::instance()->publishFinishedPageOpened();
    }

    return true;
}

bool GrpcOperation::undoOperation()
{
    return true;
}

bool GrpcOperation::testOperation()
{
    return true;
}

