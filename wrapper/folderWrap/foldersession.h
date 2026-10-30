#pragma once
#include "directory/directorylister.h"
#include "tasks/backgroundjob.h"
#include "tabs/tabmanager.h"
#include "watcher/folderwatcher.h"

#include "foldermodel.h"

#include <QElapsedTimer>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>

// One tab for QML: its folder, its navigation and its selection. The list of rows is `model`.
// Folders are read on a background thread; the result comes back here on the UI thread.
class FolderSession : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("TabsController makes these")

    Q_PROPERTY(FolderModel* model READ model CONSTANT)
    Q_PROPERTY(QString path READ path NOTIFY locationChanged)
    Q_PROPERTY(QString title READ title NOTIFY locationChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY locationChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY locationChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY locationChanged)
    Q_PROPERTY(bool isTrash READ isTrash NOTIFY locationChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY stateChanged)
    Q_PROPERTY(int headerKind READ headerKind NOTIFY stateChanged)         // 0 normal, 1 the Trash, 2 shows each item's folder
    Q_PROPERTY(bool canWrite READ canWrite NOTIFY locationChanged)         // whether new items can be put here
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY stateChanged)
    Q_PROPERTY(QString itemsText READ itemsText NOTIFY stateChanged)
    Q_PROPERTY(QString diskText READ diskText NOTIFY stateChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QString selectionText READ selectionText NOTIFY selectionChanged)
    Q_PROPERTY(int currentRow READ currentRow NOTIFY selectionChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap currentItem READ currentItem NOTIFY selectionChanged)    // {name, type, size, date, category}
    Q_PROPERTY(int sortKey READ sortKey NOTIFY sortChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending NOTIFY sortChanged)

public:
    FolderSession(std::shared_ptr<Tab> tab, bool showHidden, QObject* parent = nullptr);
    ~FolderSession() override;

    FolderModel* model() { return &m_model; }
    QString path() const;
    QString title() const;
    bool canGoBack() const { return m_tab->history.canGoBack(); }
    bool canGoForward() const { return m_tab->history.canGoForward(); }
    bool canGoUp() const;
    bool isTrash() const;
    bool canWrite() const;
    bool isSearching() const { return m_searching; }
    int headerKind() const;
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    int itemCount() const { return m_tab->view.count(); }
    QString itemsText() const;
    QString diskText() const { return m_diskText; }
    int selectedCount() const { return m_tab->view.selectedCount(); }
    QString selectionText() const;
    int currentRow() const { return m_tab->view.currentRow(); }
    QString currentPath() const;
    QVariantMap currentItem() const;
    int sortKey() const { return static_cast<int>(m_tab->view.sort().key); }
    bool sortAscending() const { return m_tab->view.sort().ascending; }

    const std::shared_ptr<Tab>& tab() const { return m_tab; }
    void setShowHidden(bool show);
    void setCutPaths(const QStringList& paths);
    void setActive(bool active);           // a tab in the background reads its folder when it comes forward
    void stop();                           // the tab is closing: no more listings or change reports

    Q_INVOKABLE void navigate(const QString& path);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshAndSelect(const QString& name);

    Q_INVOKABLE void openRow(int row);                       // double click or Enter
    Q_INVOKABLE void openCurrent();
    Q_INVOKABLE void click(int row, int modifiers);
    Q_INVOKABLE void contextClick(int row);                  // keeps a selection the row is part of
    Q_INVOKABLE void moveCurrent(int delta, bool extend);
    Q_INVOKABLE void moveCurrentTo(int row, bool extend);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void typeAhead(const QString& text);
    Q_INVOKABLE void sortBy(int key);                        // the same column again flips the order
    Q_INVOKABLE QStringList selectedPaths() const;           // the selection, or the folder itself if nothing is selected
    Q_INVOKABLE QStringList selectedItems() const;           // only what is selected: anything that changes files uses this
    Q_INVOKABLE QString selectedUrlText() const;             // for dragging: file URLs, one per line
    Q_INVOKABLE void setFilter(const QString& text);         // narrows this folder as you type
    Q_INVOKABLE void search(const QString& text);            // everything below this folder
    Q_INVOKABLE void endSearch();
    Q_INVOKABLE void selectRows(const QVariantList& rows, bool add);
    Q_INVOKABLE void copyPaths();                            // the selection's full paths, one per line

signals:
    void locationChanged();
    void stateChanged();
    void selectionChanged();
    void sortChanged();
    void aboutToRefresh();                 // the rows are about to be replaced in the same folder
    void refreshed();
    void message(const QString& text);     // something the status bar should say for a moment

private:
    void load(const std::string& focusName);
    void finishLoad(const std::shared_ptr<ListResult>& result, quint64 generation, const std::string& focusName);
    void folderChanged();
    void changeLocation(const std::string& oldPath);
    void selectionUpdated();

    std::shared_ptr<Tab> m_tab;
    FolderModel m_model;
    DirectoryLister m_lister;
    FolderWatcher m_watcher;
    BackgroundJob m_searchJob;
    std::string m_watchedPath;
    std::string m_shownPath;               // the folder whose rows are on screen

    quint64 m_generation = 0;              // answers to older listings are dropped
    bool m_loading = false;
    bool m_loaded = false;
    bool m_active = false;
    bool m_stale = false;                  // the folder changed while this tab was in the background
    bool m_searching = false;              // the rows are search results
    QString m_error;
    QString m_diskText;

    QString m_typed;                       // type-to-select
    QElapsedTimer m_typedTimer;
};
