#include "appscontroller.h"

#include "apps/desktopapps.h"
#include "apps/filetypegroups.h"
#include "magic/magicfile.h"

#include <QMetaObject>
#include <algorithm>
#include <map>

namespace {

QVariantList toList(const std::vector<DesktopApp>& apps)
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(apps.size()));
    for (const DesktopApp& app : apps) {
        list.append(QVariantMap {
            {"id", QString::fromStdString(app.id)},
            {"name", QString::fromStdString(app.name)},
            {"icon", QString::fromStdString(app.icon)},
        });
    }
    return list;
}

QStringList toStringList(const std::vector<std::string>& items)
{
    QStringList list;
    for (const std::string& item : items) list << QString::fromStdString(item);
    return list;
}

std::vector<std::string> toStd(const QStringList& items)
{
    std::vector<std::string> list;
    for (const QString& item : items) list.push_back(item.toStdString());
    return list;
}

// the type xdg-open will go by; the app's own detection stands in if that can't be asked
std::string typeForOpening(const std::string& path)
{
    const std::string type = DesktopApps::openingType(path);
    return type.empty() ? MagicFile::mimeType(path) : type;
}

// every category with each type's current default. Slow: this runs in the background.
QVariantList readFileTypes()
{
    std::vector<std::string> mains;
    for (const FileTypeGroup& group : fileTypeGroups())
        for (const FileTypeChoice& type : group.types) mains.push_back(type.mimeTypes.front());
    const std::vector<std::string> defaults = DesktopApps::defaultsFor(mains);

    std::map<std::string, DesktopApp> apps;
    for (DesktopApp& app : DesktopApps::all()) apps[app.id] = std::move(app);

    QVariantList groups;
    std::size_t next = 0;
    for (const FileTypeGroup& group : fileTypeGroups()) {
        QVariantList types;
        QStringList everyType;
        for (const FileTypeChoice& type : group.types) {
            const std::string& id = defaults[next++];
            const auto app = apps.find(id);
            const QStringList mimeTypes = toStringList(type.mimeTypes);
            everyType += mimeTypes;
            types.append(QVariantMap {
                {"label", QString::fromStdString(type.label)},
                {"mimeTypes", mimeTypes},
                {"appId", QString::fromStdString(id)},
                {"appName", app != apps.end() ? QString::fromStdString(app->second.name) : QString::fromStdString(id)},
                {"appIcon", app != apps.end() ? QString::fromStdString(app->second.icon) : QString()},
            });
        }
        groups.append(QVariantMap {{"name", QString::fromStdString(group.name)}, {"mimeTypes", everyType}, {"types", types}});
    }
    return groups;
}

} // namespace

AppsController::AppsController(QObject* parent)
    : QObject(parent)
{
}

QVariantList AppsController::appsFor(const QString& path) const
{
    return toList(DesktopApps::forMimeType(typeForOpening(path.toStdString())));
}

QVariantList AppsController::appsForTypes(const QStringList& mimeTypes) const
{
    return toList(DesktopApps::forMimeTypes(toStd(mimeTypes)));
}

QVariantMap AppsController::typeOf(const QString& path) const
{
    const std::string type = typeForOpening(path.toStdString());
    return {
        {"label", QString::fromStdString(fileTypeLabel(type))},
        {"mimeType", QString::fromStdString(type)},
        {"defaultApp", QString::fromStdString(DesktopApps::defaultFor(type))},
    };
}

QString AppsController::defaultApp(const QString& mimeType) const
{
    return QString::fromStdString(DesktopApps::defaultFor(mimeType.toStdString()));
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

void AppsController::loadFileTypes()
{
    m_load.start([this](const BackgroundJob::Handle& handle) {
        const QVariantList groups = readFileTypes();
        handle.post([this, groups] {
            QMetaObject::invokeMethod(this, [this, groups] {
                m_fileTypes = groups;
                emit fileTypesChanged();
            }, Qt::QueuedConnection);
        });
    });
}

void AppsController::setDefault(const QString& appId, const QStringList& mimeTypes, bool onlySupported)
{
    std::vector<std::string> types = toStd(mimeTypes);
    if (onlySupported) {
        const std::vector<DesktopApp> apps = DesktopApps::all();
        const auto app = std::find_if(apps.begin(), apps.end(), [&appId](const DesktopApp& item) { return item.id == appId.toStdString(); });
        if (app == apps.end()) return;
        std::erase_if(types, [&app](const std::string& type) {
            return std::find(app->mimeTypes.begin(), app->mimeTypes.end(), type) == app->mimeTypes.end();
        });
    }
    if (!DesktopApps::setDefault(appId.toStdString(), types)) emit message(QStringLiteral("The default app was not changed."));
    if (!m_fileTypes.isEmpty()) loadFileTypes();
}

bool AppsController::openTerminal(const QString& folder) const
{
    return DesktopApps::openTerminal(folder.toStdString());
}
