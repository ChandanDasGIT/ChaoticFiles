#pragma once
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// The start of a text file for the preview panel (PreviewController in QML), read off the UI thread,
// and what quick look needs to show an image.
class PreviewController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    explicit PreviewController(QObject* parent = nullptr);

    QString text() const { return m_text; }
    Q_INVOKABLE void showText(const QString& path);       // empty text if it is not a text file
    Q_INVOKABLE void clear();
    // {url, animated, width, height} of an image, read from its header; width and height are as it is shown, turned upright
    Q_INVOKABLE QVariantMap imageDetails(const QString& path) const;

signals:
    void textChanged();

private:
    BackgroundJob m_job;
    QString m_text;
    quint64 m_generation = 0;
};
