#include "foldermodel.h"
#include "wrapper/appsWrap/fileicons.h"

#include "filetype/filetype.h"
#include "foldericons/foldericons.h"
#include "folderview/folderview.h"
#include "format/format.h"

FolderModel::FolderModel(const FolderView* view, QObject* parent)
    : QAbstractListModel(parent), m_view(view)
{
}

int FolderModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_view->count();
}

QVariant FolderModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_view->count()) return {};
    const int row = index.row();
    const Entry& entry = m_view->at(row);

    switch (role) {
    case NameRole: return QString::fromStdString(entry.name);
    case PathRole: return QString::fromStdString(entry.path);
    case TypeRole: return QString::fromStdString(entry.location.empty() ? entry.typeName : entry.location);
    case SizeRole:
        return entry.isDirectory() || entry.brokenLink ? QString() : QString::fromStdString(Format::size(entry.size));
    case ModifiedRole: return entry.modified ? QString::fromStdString(Format::date(entry.modified)) : QString();
    case CategoryRole: return QString::fromLatin1(categoryName(entry.category));
    case IsDirectoryRole: return entry.isDirectory();
    case IsLinkRole: return entry.symlink;
    case IsHiddenRole: return entry.hidden;
    case IsBrokenRole: return entry.brokenLink;
    case IsSelectedRole: return m_view->isSelected(row);
    case IsCurrentRole: return m_view->currentRow() == row;
    case IsCutRole: return m_view->isCut(row);
    case IconKeyRole: return QString::fromStdString(FolderIcons::iconOf(entry.customIcon));
    case ColorKeyRole: return QString::fromStdString(FolderIcons::colorOf(entry.customIcon));
    case MimeIconRole: return FileIcons::themeIconFor(entry);
    default: return {};
    }
}

QHash<int, QByteArray> FolderModel::roleNames() const
{
    return {
        {NameRole, "name"}, {PathRole, "path"}, {TypeRole, "typeText"}, {SizeRole, "sizeText"},
        {ModifiedRole, "modifiedText"}, {CategoryRole, "category"}, {IsDirectoryRole, "isDirectory"},
        {IsLinkRole, "isLink"}, {IsHiddenRole, "isHidden"}, {IsBrokenRole, "isBroken"},
        {IsSelectedRole, "isSelected"}, {IsCurrentRole, "isCurrent"}, {IsCutRole, "isCut"},
        {IconKeyRole, "iconKey"}, {ColorKeyRole, "colorKey"}, {MimeIconRole, "mimeIcon"},
    };
}

void FolderModel::selectionChanged()
{
    if (m_view->count() == 0) return;
    emit dataChanged(index(0), index(m_view->count() - 1), {IsSelectedRole, IsCurrentRole, IsCutRole});
}
