#pragma once
#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

class FolderView;

// The rows of one tab for the file list. It owns nothing: every answer comes from the engine's FolderView.
class FolderModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("FolderSession makes these")

public:
    enum Role {
        NameRole = Qt::UserRole + 1, PathRole, TypeRole, SizeRole, ModifiedRole, CategoryRole,
        IsDirectoryRole, IsLinkRole, IsHiddenRole, IsBrokenRole, IsSelectedRole, IsCurrentRole, IsCutRole,
        IconKeyRole, ColorKeyRole
    };

    explicit FolderModel(const FolderView* view, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void beginChange() { beginResetModel(); }    // around anything that changes the rows
    void endChange() { endResetModel(); }
    void selectionChanged();

private:
    const FolderView* m_view;
};
