#include "wrapper/appsWrap/fileicons.h"

#include "entry/entry.h"
#include "mimetypes/mimetypes.h"

#include <QHash>
#include <QIcon>

namespace FileIcons {

QString themeIconFor(const Entry& entry)
{
    if (entry.isDirectory() || entry.kind != EntryKind::File || entry.brokenLink) return {};
    const std::string mimeType = MimeTypes::fromName(entry.name);
    if (mimeType.empty()) return {};

    static QHash<QString, QString> found;                              // rows ask often, and this runs on the UI thread
    const QString key = QString::fromStdString(mimeType);
    const auto known = found.constFind(key);
    if (known != found.constEnd()) return *known;

    QString icon;
    for (const std::string& name : MimeTypes::iconNames(mimeType)) {
        const QString candidate = QString::fromStdString(name);
        if (QIcon::hasThemeIcon(candidate)) {
            icon = candidate;
            break;
        }
    }
    found.insert(key, icon);
    return icon;
}

} // namespace FileIcons
