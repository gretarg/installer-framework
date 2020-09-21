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
#ifndef INSTALLEREVENTOPERATION_H
#define INSTALLEREVENTOPERATION_H

#include "qinstallerglobal.h"
#include "httpthreadcontroller.h"

#include <QtCore/QObject>
#include <QNetworkReply>
#include <pdm/include/pdm.h>

namespace QInstaller {

static bool s_isChina;
static QString s_version;
static QByteArray s_sessionId;
static QSharedPointer<QNetworkAccessManager> s_networkAccessManager;
static QThread* s_thread;
static QList<QByteArray>* s_httpData;
static QSharedPointer<HttpThreadController> s_httpThreadController;

class INSTALLER_EXPORT InstallerEventOperation : public QObject, public Operation
{
    Q_OBJECT

public:
    explicit InstallerEventOperation(PackageManagerCore *core);

    void backup();
    bool performOperation();
    bool undoOperation();
    bool testOperation();

private Q_SLOTS:
    void onError(QNetworkReply::NetworkError code);
    void onFinished();
private:
    void populateLauncherEvent(QJsonObject& launcherEven, QString messageTypet);
    void populatePayload(QJsonObject& payload, QString messageType);
    void populateApplication(QJsonObject& application);
    void populatePdm(QJsonObject& pdm);
    void populateFromPdmSubItem(PDM::SubItem subitem, QJsonObject& subItemJson);
    void populateSubItem(QJsonObject& pdm);
    void populateStarted(QJsonObject& started);
    void send();
};

} // namespace

#endif
