#pragma once
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

// Cut, copy and paste of files (ClipboardController in QML), in the format other Linux file managers use,
// so files can be pasted between them.
class ClipboardController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool canPaste READ canPaste NOTIFY changed)

public:
    explicit ClipboardController(QObject* parent = nullptr);

    bool canPaste() const;
    Q_INVOKABLE void copy(const QStringList& paths);
    Q_INVOKABLE void cut(const QStringList& paths);
    Q_INVOKABLE void paste(const QString& destination);

signals:
    void changed();

private:
    void put(const QStringList& paths, bool cutting);
};
