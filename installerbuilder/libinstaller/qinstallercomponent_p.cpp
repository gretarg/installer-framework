/**************************************************************************
**
** This file is part of Qt SDK**
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).*
**
** Contact:  Nokia Corporation qt-info@nokia.com**
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception version
** 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you are unsure which license is appropriate for your use, please contact
** (qt-info@nokia.com).
**
**************************************************************************/
#include "qinstallercomponent_p.h"

#include "messageboxhandler.h"
#include "qinstaller.h"
#include "qinstallercomponent.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>

namespace QInstaller {

// -- ComponentPrivate

ComponentPrivate::ComponentPrivate(Installer* installer, Component* qq)
    : q(qq),
    m_installer(installer),
    m_parentComponent(0),
    m_licenseOperation(0),
    m_minimumProgressOperation(0),
    m_newlyInstalled (false),
    m_operationsCreated(false),
    m_removeBeforeUpdate(true),
    m_autoCreateOperations(true),
    m_operationsCreatedSuccessfully(true)
{
}

void ComponentPrivate::init()
{
    // register translation stuff
    m_scriptEngine.installTranslatorFunctions();

    // register QMessageBox::StandardButton enum in the script connection
    registerMessageBox(&m_scriptEngine);

    // register QDesktopServices in the script connection
    QScriptValue desktopServices = m_scriptEngine.newArray();
    setProperty(desktopServices, QLatin1String("DesktopLocation"), QDesktopServices::DesktopLocation);
    setProperty(desktopServices, QLatin1String("DocumentsLocation"), QDesktopServices::DocumentsLocation);
    setProperty(desktopServices, QLatin1String("FontsLocation"), QDesktopServices::FontsLocation);
    setProperty(desktopServices, QLatin1String("ApplicationsLocation"), QDesktopServices::ApplicationsLocation);
    setProperty(desktopServices, QLatin1String("MusicLocation"), QDesktopServices::MusicLocation);
    setProperty(desktopServices, QLatin1String("MoviesLocation"), QDesktopServices::MoviesLocation);
    setProperty(desktopServices, QLatin1String("PicturesLocation"), QDesktopServices::PicturesLocation);
    setProperty(desktopServices, QLatin1String("TempLocation"), QDesktopServices::TempLocation);
    setProperty(desktopServices, QLatin1String("HomeLocation"), QDesktopServices::HomeLocation);
    setProperty(desktopServices, QLatin1String("DataLocation"), QDesktopServices::DataLocation);
    setProperty(desktopServices, QLatin1String("CacheLocation"), QDesktopServices::CacheLocation);

    desktopServices.setProperty(QLatin1String("openUrl"), m_scriptEngine.newFunction(qDesktopServicesOpenUrl));
    desktopServices.setProperty(QLatin1String("displayName"),
        m_scriptEngine.newFunction(qDesktopServicesDisplayName));
    desktopServices.setProperty(QLatin1String("storageLocation"),
        m_scriptEngine.newFunction(qDesktopServicesStorageLocation));

    // register ::WizardPage enum in the script connection
    QScriptValue qinstaller = m_scriptEngine.newArray();
    setProperty(qinstaller, QLatin1String("Introduction"), Installer::Introduction);
    setProperty(qinstaller, QLatin1String("LicenseCheck"), Installer::LicenseCheck);
    setProperty(qinstaller, QLatin1String("TargetDirectory"), Installer::TargetDirectory);
    setProperty(qinstaller, QLatin1String("ComponentSelection"), Installer::ComponentSelection);
    setProperty(qinstaller, QLatin1String("StartMenuSelection"), Installer::StartMenuSelection);
    setProperty(qinstaller, QLatin1String("ReadyForInstallation"), Installer::ReadyForInstallation);
    setProperty(qinstaller, QLatin1String("PerformInstallation"), Installer::PerformInstallation);
    setProperty(qinstaller, QLatin1String("InstallationFinished"), Installer::InstallationFinished);
    setProperty(qinstaller, QLatin1String("End"), Installer::End);

    // register ::Status enum in the script connection
    setProperty(qinstaller, QLatin1String("InstallerSuccess"), Installer::Success);
    setProperty(qinstaller, QLatin1String("InstallerSucceeded"), Installer::Success);
    setProperty(qinstaller, QLatin1String("InstallerFailed"), Installer::Failure);
    setProperty(qinstaller, QLatin1String("InstallerFailure"), Installer::Failure);
    setProperty(qinstaller, QLatin1String("InstallerRunning"), Installer::Running);
    setProperty(qinstaller, QLatin1String("InstallerCanceled"), Installer::Canceled);
    setProperty(qinstaller, QLatin1String("InstallerCanceledByUser"), Installer::Canceled);
    setProperty(qinstaller, QLatin1String("InstallerUnfinished"), Installer::Unfinished);

    QScriptValue installerObject = m_scriptEngine.newQObject(m_installer);
    installerObject.setProperty(QLatin1String("componentByName"),
        m_scriptEngine.newFunction(qInstallerComponentByName, 1));

    m_scriptEngine.globalObject().setProperty(QLatin1String("QInstaller"), qinstaller);
    m_scriptEngine.globalObject().setProperty(QLatin1String("installer"), installerObject);
    m_scriptEngine.globalObject().setProperty(QLatin1String("QDesktopServices"), desktopServices);
    m_scriptEngine.globalObject().setProperty(QLatin1String("component"), m_scriptEngine.newQObject(q));
}

void ComponentPrivate::setProperty(QScriptValue &scriptValue, const QString &propertyName, int value)
{
    scriptValue.setProperty(propertyName, m_scriptEngine.newVariant(value));
}


// -- ComponentModelHelper

ComponentModelHelper::ComponentModelHelper()
{
    setCheckState(Qt::Unchecked);
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
}

ComponentModelHelper::~ComponentModelHelper()
{
}

/*!
    Returns the number of child components.
*/
int ComponentModelHelper::childCount() const
{
    if (m_componentPrivate->m_installer->virtualComponentsVisible())
        return m_componentPrivate->m_allComponents.count();
    return m_componentPrivate->m_components.count();
}

/*!
    Returns the index of this component as seen from it's parent.
*/
int ComponentModelHelper::indexInParent() const
{
    int index = 0;
    if (Component *parent = m_componentPrivate->m_parentComponent->parentComponent())
        index = parent->childComponents(false, AllMode).indexOf(m_componentPrivate->m_parentComponent);
    return (index >= 0 ? index : 0);
}

/*!
    Returns all children and whose children depending if virtual components are visible or not.
*/
QList<Component*> ComponentModelHelper::childs() const
{
    QList<Component*> *components = &m_componentPrivate->m_components;
    if (m_componentPrivate->m_installer->virtualComponentsVisible())
        components = &m_componentPrivate->m_allComponents;

    QList<Component*> result;
    foreach (Component *component, *components) {
        result.append(component);
        result += component->childComponents(true, AllMode);
    }
    return result;
}

/*!
    Returns the component at index position in the list. Index must be a valid position in
    the list (i.e., index >= 0 && index < childCount()). Otherwise it returns 0.
*/
Component* ComponentModelHelper::childAt(int index) const
{
    if (index >= 0 && index < childCount()) {
        if (m_componentPrivate->m_installer->virtualComponentsVisible())
            return m_componentPrivate->m_allComponents.value(index, 0);
        return m_componentPrivate->m_components.value(index, 0);
    }
    return 0;
}

/*!
    Determines if the components installations status can be changed. The default value is true.
*/
bool ComponentModelHelper::isEnabled() const
{
    return (flags() & Qt::ItemIsEnabled) != 0;
}

/*!
    Enables oder disables ability to change the components installations status.
*/
void ComponentModelHelper::setEnabled(bool enabled)
{
    changeFlags(enabled, Qt::ItemIsEnabled);
}

/*!
    Returns whether the component is tristate; that is, if it's checkable with three separate states.
    The default value is false.
*/
bool ComponentModelHelper::isTristate() const
{
    return (flags() & Qt::ItemIsTristate) != 0;
}

/*!
    Sets whether the component is tristate. If tristate is true, the component is checkable with three
    separate states; otherwise, the component is checkable with two states.

    (Note that this also requires that the component is checkable; see isCheckable().)
*/
void ComponentModelHelper::setTristate(bool tristate)
{
    changeFlags(tristate, Qt::ItemIsTristate);
}

/*!
    Returns whether the component is user-checkable. The default value is true.
*/
bool ComponentModelHelper::isCheckable() const
{
    return (flags() & Qt::ItemIsUserCheckable) != 0;
}

/*!
    Sets whether the component is user-checkable. If checkable is true, the component can be checked by the
    user; otherwise, the user cannot check the component. The delegate will render a checkable component
    with a check box next to the component's text.
*/
void ComponentModelHelper::setCheckable(bool checkable)
{
    if (checkable && !isCheckable()) {
        // make sure there's data for the checkstate role
        if (!data(Qt::CheckStateRole).isValid())
            setData(Qt::Unchecked, Qt::CheckStateRole);
    }
    changeFlags(checkable, Qt::ItemIsUserCheckable);
}

/*!
    Returns whether the component is selectable by the user. The default value is true.
*/
bool ComponentModelHelper::isSelectable() const
{
    return (flags() & Qt::ItemIsSelectable) != 0;
}

/*!
    Sets whether the component is selectable. If selectable is true, the component can be selected by the
    user; otherwise, the user cannot select the component.
*/
void ComponentModelHelper::setSelectable(bool selectable)
{
    changeFlags(selectable, Qt::ItemIsSelectable);
}

/*!
    Returns the item flags for the component. The item flags determine how the user can interact with the
    component.
*/
Qt::ItemFlags ComponentModelHelper::flags() const
{
    QVariant variant = data(Qt::UserRole - 1);
    if (!variant.isValid())
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable| Qt::ItemIsUserCheckable);
    return Qt::ItemFlags(variant.toInt());
}

/*!
    Sets the item flags for the component to flags. The item flags determine how the user can interact with
    the component. This is often used to disable an component.
*/
void ComponentModelHelper::setFlags(Qt::ItemFlags flags)
{
    setData(int(flags), Qt::UserRole - 1);
}

/*!
    Returns the checked state of the component.
*/
Qt::CheckState ComponentModelHelper::checkState() const
{
    return Qt::CheckState(qvariant_cast<int>(data(Qt::CheckStateRole)));
}

/*!
    Sets the check state of the component to be state.
*/
void ComponentModelHelper::setCheckState(Qt::CheckState state)
{
    setData(state, Qt::CheckStateRole);
}

/*!
    Returns the components's data for the given role, or an invalid QVariant if there is no data for role.
*/
QVariant ComponentModelHelper::data(int role) const
{
    return m_values.value((role == Qt::EditRole ? Qt::DisplayRole : role), QVariant());
}

/*!
    Sets the component's data for the given role to the specified value.
*/
void ComponentModelHelper::setData(const QVariant &value, int role)
{
    m_values.insert((role == Qt::EditRole ? Qt::DisplayRole : role), value);
}

// -- protected

void ComponentModelHelper::setPrivate(ComponentPrivate *componentPrivate)
{
    m_componentPrivate = componentPrivate;
}

// -- private

void ComponentModelHelper::changeFlags(bool enable, Qt::ItemFlags itemFlags)
{
    setFlags(enable ? flags() |= itemFlags : flags() &= ~itemFlags);
}

}   // namespace QInstaller
