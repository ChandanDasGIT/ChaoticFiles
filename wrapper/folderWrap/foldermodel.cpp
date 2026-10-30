#include "foldermodel.h"

#include "filetype/filetype.h"
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
    case TypeRole: return QString::fromStdString(entry.typeName);
    case SizeRole:
        return entry.isDirectory() || entry.brokenLink ? QString() : QString::fromStdString(Format::size(entry.size));
    case ModifiedRole: return QString::fromStdString(Format::date(entry.modified));
    case CategoryRole: return QString::fromLatin1(categoryName(entry.category));
    case IsDirectoryRole: return entry.isDirectory();
    case IsLinkRole: return entry.symlink;
    case IsHiddenRole: return entry.hidden;
    case IsBrokenRole: return entry.brokenLink;
    case IsSelectedRole: return m_view->isSelected(row);
    case IsCurrentRole: return m_view->currentRow() == row;
    default: return {};
    }
}

QHash<int, QByteArray> FolderModel::roleNames() const
{
    return {
        {NameRole, "name"}, {PathRole, "path"}, {TypeRole, "typeText"}, {SizeRole, "sizeText"},
        {ModifiedRole, "modifiedText"}, {CategoryRole, "category"}, {IsDirectoryRole, "isDirectory"},
        {IsLinkRole, "isLink"}, {IsHiddenRole, "isHidden"}, {IsBrokenRole, "isBroken"},
        {IsSelectedRole, "isSelected"}, {IsCurrentRole, "isCurrent"},
    };
}

void FolderModel::selectionChanged()
{
    if (m_view->count() == 0) return;
    emit dataChanged(index(0), index(m_view->count() - 1), {IsSelectedRole, IsCurrentRole});
}
