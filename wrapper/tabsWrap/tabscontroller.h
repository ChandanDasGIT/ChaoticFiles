#pragma once
#include "tabs/tabmanager.h"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class FolderSession;
class QQmlEngine;
class QJSEngine;

// The open tabs (TabsController in QML): a list model of them, and everything that adds, closes or moves one.
class TabsController : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int currentIndex READ currentIndex WRITE selectTab NOTIFY currentChanged)
    Q_PROPERTY(FolderSession* currentSession READ currentSession NOTIFY currentChanged)
    Q_PROPERTY(bool canReopen READ canReopen NOTIFY reopenChanged)

public:
    enum Role { SessionRole = Qt::UserRole + 1, IsCurrentRole };

    static TabsController* instance();
    static TabsController* create(QQmlEngine*, QJSEngine*);

    // the first tabs: the folders given, or else what the settings say. A folder that is gone opens Home.
    void start(const QStringList& folders, int current = 0);
    QStringList folders() const;                             // where each tab is, in order

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentIndex() const { return m_tabs.current(); }
    FolderSession* currentSession() const;
    bool canReopen() const { return m_tabs.canReopen(); }

    Q_INVOKABLE void selectTab(int index);
    Q_INVOKABLE void newTab();                               // a new tab at Home, right after the current one
    Q_INVOKABLE void openInBackground(const QString& path);  // a new tab that does not take over
    Q_INVOKABLE void closeTab(int index);                    // closing the last tab closes the window
    Q_INVOKABLE void closeCurrent();
    Q_INVOKABLE void nextTab();
    Q_INVOKABLE void previousTab();
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void reopenTab();

    void setCutPaths(const QStringList& paths);

signals:
    void currentChanged();
    void reopenChanged();
    void message(const QString& text);

private:
    explicit TabsController(QObject* parent);

    FolderSession* insertTab(const std::shared_ptr<Tab>& tab, int index);
    void updateCurrent(FolderSession* previous);

    TabManager m_tabs;
    QList<FolderSession*> m_sessions;
    QStringList m_cutPaths;
};
