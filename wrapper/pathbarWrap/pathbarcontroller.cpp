#include "pathbarcontroller.h"

#include "pathbar/pathbar.h"
#include "settingscontroller.h"

namespace {

constexpr std::size_t kMaxSuggestions = 8;

QVariantList toList(const std::vector<PathPart>& parts)
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(parts.size()));
    for (const PathPart& part : parts) {
        list.append(QVariantMap {
            {"label", QString::fromStdString(part.label)},
            {"path", QString::fromStdString(part.path)},
        });
    }
    return list;
}

} // namespace

PathBarController::PathBarController(QObject* parent)
    : QObject(parent)
{
}

QVariantList PathBarController::parts(const QString& path) const
{
    return toList(PathBar::split(path.toStdString()));
}

QVariantList PathBarController::subfolders(const QString& path) const
{
    return toList(PathBar::subfolders(path.toStdString(), SettingsController::instance()->showHidden()));
}

QString PathBarController::resolve(const QString& typed, const QString& currentFolder) const
{
    return QString::fromStdString(PathBar::resolve(typed.toStdString(), currentFolder.toStdString()));
}

QVariantList PathBarController::suggestions(const QString& typed, const QString& currentFolder) const
{
    return toList(PathBar::suggestions(typed.toStdString(), currentFolder.toStdString(),
                                       SettingsController::instance()->showHidden(), kMaxSuggestions));
}
