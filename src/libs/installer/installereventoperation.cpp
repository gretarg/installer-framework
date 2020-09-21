/**************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Installer Framework.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
**************************************************************************/

#include "installereventoperation.h"

#include "packagemanagercore.h"
#include "globals.h"

#include "eventlogger.h"

#include "pdm/src/utilities.h"

#include <QtCore/QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QThread>
#include <QAbstractEventDispatcher>
#include <QTimer>

using namespace QInstaller;

InstallerEventOperation::InstallerEventOperation(PackageManagerCore *core)
    : UpdateOperation(core)
{
    setName(QLatin1String("InstallerEvent"));
}

void InstallerEventOperation::backup()
{
}

bool InstallerEventOperation::performOperation()
{
    if (!checkArgumentCount(1, 3, tr(" ([\"init\"|message type], [\"china\"|\"world\"] (init only), version (init only))")))
        return false;

    const QStringList args = arguments();
    const QString action = args.at(0);

    if(action == QString::fromLatin1("init"))
    {
        if (!checkArgumentCount(3, 3, tr(" ([\"init\"|message type], [\"china\"|\"world\"] (init only), version (init only))")))
            return false;

        s_isChina = args.at(1) == QString::fromLatin1("china");
        s_version = args.at(2);

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

        s_sessionId = hasher.result().toBase64();
    }
    else
    {
        QJsonObject launcherEvent;
        populateLauncherEvent(launcherEvent, action);

        QJsonObject drasl;

        drasl[QLatin1String("action")] = action;
        drasl[QLatin1String("hasDispatcher")] = QThread::currentThread()->eventDispatcher() ? QLatin1String("true"):QLatin1String("false");
        drasl[QLatin1String("json")] = launcherEvent;

        for(int i=0;i<args.length();i++)
        {
            drasl[QString::fromLatin1("arg%1").arg(i)] = args.at(i);
        }

        QJsonDocument doc(drasl);

        /*if(!s_networkAccessManager)
        {
            s_thread = new QThread();
            s_thread->start();
            s_networkAccessManager.reset(new QNetworkAccessManager());
            s_httpData = new QList<QByteArray>();
            //s_networkAccessManager->moveToThread(s_thread);
        }*/

        /*HttpThread* thread = new HttpThread();
        thread->start();
        thread->queue(s_httpData->last());*/

        QEventLoop synchronous;
        QNetworkRequest request;

        connect(s_networkAccessManager.data(), SIGNAL(finished(QNetworkReply*)), &synchronous, SLOT(quit()));
        /*//connect(s_networkAccessManager.data(), &QNetworkAccessManager::finished, this, [](){});
        request.setUrl(QUrl(QLatin1String("https://localhost:5001/weatherforecast")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));

        s_httpData->append(doc.toJson());
        s_networkAccessManager->post(request, s_httpData->last());

        synchronous.exec();*/

        if(!s_httpThreadController.data())
        {
            s_httpThreadController.reset(new HttpThreadController);
        }

        s_httpThreadController->operate(doc.toJson());


        /*QUrl url = QUrl(QLatin1String("https://localhost:5001/weatherforecast"));

        //QNetworkAccessManager * mgr = new QNetworkAccessManager(this);

        connect(s_networkAccessManager.data(),SIGNAL(finished(QNetworkReply*)),this,SLOT(onFinish(QNetworkReply*)));
        //connect(s_networkAccessManager.data(),SIGNAL(finished(QNetworkReply*)),mgr,SLOT(deleteLater()));

        s_networkAccessManager.data()->post(QNetworkRequest(url), doc.toJson());*/
    }

    return true;
}

void InstallerEventOperation::send()
{
    QNetworkAccessManager* nam = new QNetworkAccessManager();
    QEventLoop synchronous;
    QNetworkRequest request;

    connect(nam, SIGNAL(finished(QNetworkReply*)), &synchronous, SLOT(quit()));
    request.setUrl(QUrl(QLatin1String("https://localhost:5001/weatherforecast")));
    //request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));

    QNetworkReply* reply = nam->post(request, "{ \"action\": \"drasl\", \"json\":{}}");

    connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
}

bool InstallerEventOperation::undoOperation()
{
    return true;
}

bool InstallerEventOperation::testOperation()
{
    return true;
}

void InstallerEventOperation::onError(QNetworkReply::NetworkError code)
{
    qDebug() << "Failed to send HTTP POST, error code: " << code;
}

void InstallerEventOperation::onFinished()
{
    qDebug() << "HTTP POST sent";
}

void InstallerEventOperation::populateLauncherEvent(QJsonObject& launcherEvent, QString messageType)
{
    auto payload = QJsonObject();
    populatePayload(payload, messageType);
    launcherEvent[QLatin1String("payload")] = payload;
    launcherEvent[QLatin1String("md5_session")] = QString::fromStdString(std::string(s_sessionId.data(), size_t(s_sessionId.size())));
    auto application = QJsonObject();
    populateApplication(application);
    launcherEvent[QLatin1String("application")] = application;

    QUuid osUuid = QUuid::fromString(QString::fromStdString(PDM::GetMachineUuid()));
    launcherEvent[QLatin1String("operatingSystemUuid")] = QString::fromUtf8(osUuid.toRfc4122().toBase64());
    launcherEvent[QLatin1String("timestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

void InstallerEventOperation::populatePayload(QJsonObject& payload, QString messageType)
{
    payload[QLatin1String("@type")] = QString::fromLatin1("eve_launcher.installer.%1").arg(messageType);
    auto message = QJsonObject();

    if(messageType == QString::fromLatin1("Started"))
    {
        populateStarted(message);
    }

    payload[QLatin1String("message")] = message;
}

void InstallerEventOperation::populateApplication(QJsonObject& application)
{
    application[QLatin1String("version")] = s_version;

    /*enum BuildTag {
    RELEASE = 0;
    BETA = 1;
    DEV = 2;
    }*/
    application[QLatin1String("build_tag")] = 0;

    /*enum Locale {
    WORLD = 0;
    CHINA = 1;
    }*/
    application[QLatin1String("locale")] = s_isChina ? 1 : 0;
}

void InstallerEventOperation::populatePdm(QJsonObject& pdm)
{
    auto pdmObj = PDM::RetrievePDMData("EVE Launcher Installer", s_version.toStdString());

    auto pdmData = QJsonObject();
    populateFromPdmSubItem(pdmObj.data, pdmData);
    pdm[QLatin1String("data")] = pdmData;
    pdm[QLatin1String("timestamp")] = QDateTime(QDate(pdmObj.timestamp.tm_year, pdmObj.timestamp.tm_mon, pdmObj.timestamp.tm_mday), QTime(pdmObj.timestamp.tm_hour, pdmObj.timestamp.tm_min, pdmObj.timestamp.tm_sec)).toString(Qt::ISODateWithMs);
}

void InstallerEventOperation::populateFromPdmSubItem(PDM::SubItem subitem, QJsonObject& subItemJson)
{
    if(subitem.subitems.size() > 0)
    {
        auto childSubItemsJson = QJsonArray();
        std::for_each(cbegin(subitem.subitems), cend(subitem.subitems), [this, &childSubItemsJson](const PDM::SubItem& childSubItem)
        {
            auto childSubItemJson = QJsonObject();
            populateFromPdmSubItem(childSubItem, childSubItemJson);
            childSubItemsJson.append(childSubItemJson);
        });

        subItemJson[QLatin1String("sub_items")] = childSubItemsJson;
    }

    if(subitem.items.size() > 0)
    {
        auto itemsJson = QJsonArray();
        std::for_each(cbegin(subitem.items), cend(subitem.items), [&itemsJson](const PDM::DataField& item)
        {
            auto itemJson = QJsonObject();
            itemJson[QLatin1String("name")] = QString::fromStdString(item.name);
            itemJson[QLatin1String("value")] = QString::fromStdString(item.value);
            itemsJson.append(itemJson);
        });

        subItemJson[QLatin1String("items")] = itemsJson;
    }
}

void InstallerEventOperation::populateStarted(QJsonObject& started)
{
    auto pdm = QJsonObject();
    populatePdm(pdm);
    started[QLatin1String("pdm")] = pdm;
}
