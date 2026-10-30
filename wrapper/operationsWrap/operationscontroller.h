#pragma once
#include "tasks/backgroundjob.h"
#include "transfer/transfer.h"
#include "undo/undostack.h"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <future>
#include <memory>

class QQmlEngine;
class QJSEngine;

// Everything that changes the disk (OperationsController in QML). Long work runs in the background and shows
// as a row of this list model, with progress, pause and cancel. Nothing here decides how files are handled;
// that is the engine's Transfer, Trash, FileOps and Archiver.
class OperationsController : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool hasConflict READ hasConflict NOTIFY conflictChanged)
    Q_PROPERTY(QString conflictName READ conflictName NOTIFY conflictChanged)
    Q_PROPERTY(QString conflictSource READ conflictSource NOTIFY conflictChanged)
    Q_PROPERTY(QString conflictTarget READ conflictTarget NOTIFY conflictChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoChanged)

public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, DetailRole, ProgressRole, PausedRole, CanPauseRole };

    static OperationsController* instance();
    static OperationsController* create(QQmlEngine*, QJSEngine*);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasConflict() const { return !m_conflicts.isEmpty(); }
    QString conflictName() const;
    QString conflictSource() const;       // a line about each of the two files
    QString conflictTarget() const;
    bool canUndo() const { return m_undo.canUndo(); }
    QString undoText() const;

    Q_INVOKABLE void copy(const QStringList& paths, const QString& destination);
    Q_INVOKABLE void move(const QStringList& paths, const QString& destination);
    Q_INVOKABLE void dropUrls(const QList<QUrl>& urls, const QString& destination);   // Ctrl copies, Shift moves
    Q_INVOKABLE void trash(const QStringList& paths);
    Q_INVOKABLE void deleteForever(const QStringList& paths);
    Q_INVOKABLE void restore(const QStringList& paths);
    Q_INVOKABLE void emptyTrash();
    Q_INVOKABLE void compress(const QStringList& paths, const QString& archivePath);
    Q_INVOKABLE void extract(const QStringList& archives);
    Q_INVOKABLE void createLinks(const QStringList& paths);
    Q_INVOKABLE void undo();

    // these are instant; each returns a sentence if it failed, otherwise empty text
    Q_INVOKABLE QString rename(const QString& path, const QString& newName);
    Q_INVOKABLE QString newFolder(const QString& folder, const QString& name);
    Q_INVOKABLE QString newFile(const QString& folder, const QString& name);
    Q_INVOKABLE QString nameProblem(const QString& name) const;
    Q_INVOKABLE QString freeName(const QString& folder, const QString& name) const;
    Q_INVOKABLE bool isArchive(const QString& name) const;
    Q_INVOKABLE QString withoutExtension(const QString& name) const;

    Q_INVOKABLE void pause(int id);       // pauses, or resumes when paused
    Q_INVOKABLE void cancel(int id);
    Q_INVOKABLE void answerConflict(int choice, bool applyToAll);   // 0 replace, 1 skip, 2 keep both, 3 cancel

signals:
    void conflictChanged();
    void undoChanged();
    void message(const QString& text);
    void errors(const QString& title, const QString& text);
    void trashFailed(const QStringList& paths, const QString& reason);   // these could go nowhere but gone for good
    void operationFinished();

private:
    struct Operation {
        int id = 0;
        QString title;
        QString detail;
        double progress = 0;
        bool paused = false;
        std::shared_ptr<Transfer> transfer;
        std::shared_ptr<std::atomic<bool>> cancelled;
        std::unique_ptr<BackgroundJob> job;
    };
    struct Conflict {
        int operation = 0;
        std::string source;
        std::string target;
        std::shared_ptr<std::promise<ConflictAnswer>> answer;
    };

    explicit OperationsController(QObject* parent);

    int addOperation(const QString& title);
    int rowOf(int id) const;
    void update(int id, const QString& detail, double progress);
    void finishOperation(int id);
    void transfer(TransferKind kind, const QStringList& paths, const QString& destination, UndoStep::Kind undoKind);
    void toUi(const BackgroundJob::Handle& handle, std::function<void()> work);
    void showConflict(int id, const std::string& source, const std::string& target,
                      const std::shared_ptr<std::promise<ConflictAnswer>>& answer);
    void reportProblems(const QString& title, const std::vector<std::string>& problems);

    QList<std::shared_ptr<Operation>> m_operations;
    QList<Conflict> m_conflicts;
    UndoStack m_undo;
    int m_nextId = 1;
};
