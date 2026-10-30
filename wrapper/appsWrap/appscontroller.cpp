#include "appscontroller.h"

#include "apps/desktopapps.h"
#include "magic/magicfile.h"

#include <QVariantMap>

namespace {

QVariantList toList(const std::vector<DesktopApp>& apps)
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(apps.size()));
    for (const DesktopApp& app : apps) {
        list.append(QVariantMap {{"id", QString::fromStdString(app.id)}, {"name", QString::fromStdString(app.name)}});
    }
    return list;
}

} // namespace

AppsController::AppsController(QObject* parent)
    : QObject(parent)
{
}

QVariantList AppsController::appsFor(const QString& path) const
{
    return toList(DesktopApps::forMimeType(MagicFile::mimeType(path.toStdString())));
}

QVariantList AppsController::allApps() const
{
    return toList(DesktopApps::all());
}

bool AppsController::open(const QString& appId, const QStringList& paths) const
{
    std::vector<std::string> list;
    for (const QString& path : paths) list.push_back(path.toStdString());
    return DesktopApps::open(appId.toStdString(), list);
}

bool AppsController::openTerminal(const QString& folder) const
{
    return DesktopApps::openTerminal(folder.toStdString());
}
