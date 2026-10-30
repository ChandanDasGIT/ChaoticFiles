#include "tabscontroller.h"

#include "foldersession.h"
#include "pathbar/pathbar.h"
#include "settingscontroller.h"

#include <QCoreApplication>
#include <QJSEngine>

TabsController* TabsController::instance()
{
    static TabsController* controller = new TabsController(QCoreApplication::instance());
    return controller;
}

TabsController* TabsController::create(QQmlEngine*, QJSEngine*)
{
    TabsController* controller = instance();
    QJSEngine::setObjectOwnership(controller, QJSEngine::CppOwnership);
    return controller;
}

TabsController::TabsController(QObject* parent)
    : QAbstractListModel(parent)
{
    SettingsController* settings = SettingsController::instance();
    connect(settings, &SettingsController::showHiddenChanged, this, [this, settings] {
        for (FolderSession* session : m_sessions) session->setShowHidden(settings->showHidden());
    });
}

void TabsController::start(const QStringList& folders)
{
    for (const QString& path : folders) {
        const std::string folder = PathBar::normalize(path.toStdString());
        const int place = m_tabs.count();
        insertTab(m_tabs.open(PathBar::isDirectory(folder) ? folder : PathBar::homeDirectory(), place), place);
    }
    if (m_tabs.count() == 0) insertTab(m_tabs.open(PathBar::homeDirectory(), 0), 0);

    updateCurrent(nullptr);
}

int TabsController::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_sessions.size());
}

QVariant TabsController::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_tabs.count()) return {};
    switch (role) {
    case SessionRole: return QVariant::fromValue(m_sessions.at(index.row()));
    case IsCurrentRole: return index.row() == m_tabs.current();
    default: return {};
    }
}

QHash<int, QByteArray> TabsController::roleNames() const
{
    return {{SessionRole, "session"}, {IsCurrentRole, "isCurrent"}};
}

FolderSession* TabsController::currentSession() const
{
    return m_tabs.current() >= 0 ? m_sessions.at(m_tabs.current()) : nullptr;
}

FolderSession* TabsController::insertTab(const std::shared_ptr<Tab>& tab, int index)
{
    auto* session = new FolderSession(tab, SettingsController::instance()->showHidden(), this);
    connect(session, &FolderSession::message, this, &TabsController::message);
    session->setCutPaths(m_cutPaths);
    beginInsertRows({}, index, index);
    m_sessions.insert(index, session);
    endInsertRows();
    return session;
}

void TabsController::updateCurrent(FolderSession* previous)
{
    FolderSession* session = currentSession();
    if (previous && previous != session) previous->setActive(false);
    if (session) session->setActive(true);
    if (m_tabs.count() > 0) emit dataChanged(index(0), index(m_tabs.count() - 1), {IsCurrentRole});
    emit currentChanged();
}

void TabsController::selectTab(int index)
{
    if (index == m_tabs.current() || index < 0 || index >= m_tabs.count()) return;
    FolderSession* previous = currentSession();
    m_tabs.setCurrent(index);
    updateCurrent(previous);
}

void TabsController::newTab()
{
    FolderSession* previous = currentSession();
    const int place = m_tabs.current() + 1;
    insertTab(m_tabs.open(PathBar::homeDirectory(), place), place);
    m_tabs.setCurrent(place);
    updateCurrent(previous);
}

void TabsController::openInBackground(const QString& path)
{
    FolderSession* current = currentSession();
    const int place = m_tabs.current() + 1;
    insertTab(m_tabs.open(PathBar::normalize(path.toStdString()), place), place);
    updateCurrent(current);
}

void TabsController::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.count()) return;
    if (m_tabs.count() == 1) {
        QCoreApplication::quit();
        return;
    }

    FolderSession* previous = currentSession();
    FolderSession* closing = m_sessions.at(index);
    closing->stop();

    beginRemoveRows({}, index, index);
    m_tabs.close(index);
    m_sessions.removeAt(index);
    endRemoveRows();

    closing->deleteLater();
    updateCurrent(previous == closing ? nullptr : previous);
    emit reopenChanged();
}

void TabsController::closeCurrent()
{
    closeTab(m_tabs.current());
}

void TabsController::nextTab()
{
    selectTab((m_tabs.current() + 1) % m_tabs.count());
}

void TabsController::previousTab()
{
    selectTab((m_tabs.current() + m_tabs.count() - 1) % m_tabs.count());
}

void TabsController::moveTab(int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= m_tabs.count() || to >= m_tabs.count()) return;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_tabs.move(from, to);
    m_sessions.move(from, to);
    endMoveRows();
    updateCurrent(currentSession());
}

void TabsController::setCutPaths(const QStringList& paths)
{
    m_cutPaths = paths;
    for (FolderSession* session : m_sessions) session->setCutPaths(paths);
}

void TabsController::reopenTab()
{
    if (!m_tabs.canReopen()) return;
    FolderSession* previous = currentSession();
    const int place = m_tabs.reopenIndex();
    insertTab(m_tabs.reopen(), place);
    m_tabs.setCurrent(place);
    updateCurrent(previous);
    emit reopenChanged();
}
