#ifndef GRPCEVENT_H
#define GRPCEVENT_H
#include "qinstallerglobal.h"

#include <QtCore/QObject>

namespace QInstaller {
class INSTALLER_EXPORT GrpcOperation : public QObject, public Operation
{
    Q_OBJECT
public:
    explicit GrpcOperation(PackageManagerCore *core);

    void backup();
    bool performOperation();
    bool undoOperation();
    bool testOperation();
};
}
#endif // GRPCEVENT_H
