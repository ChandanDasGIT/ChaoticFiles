#pragma once
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The address bar's questions to the engine (PathBarController in QML). It keeps no state.
class PathBarController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PathBarController(QObject* parent = nullptr);

    Q_INVOKABLE QVariantList parts(const QString& path) const;                       // [{label, path}, ...]
    Q_INVOKABLE QVariantList subfolders(const QString& path) const;
    Q_INVOKABLE QString resolve(const QString& typed, const QString& currentFolder) const;
    Q_INVOKABLE QVariantList suggestions(const QString& typed, const QString& currentFolder) const;
};
