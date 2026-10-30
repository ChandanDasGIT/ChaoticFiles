#pragma once
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>

// The start of a text file for the preview panel (PreviewController in QML), read off the UI thread.
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

signals:
    void textChanged();

private:
    BackgroundJob m_job;
    QString m_text;
    quint64 m_generation = 0;
};
