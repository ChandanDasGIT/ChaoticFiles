#pragma once
#include "properties/propertyreport.h"
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <functional>

// The properties window (PropertiesController in QML). Quick facts arrive first and the window fills in
// from a background thread, so opening it can never freeze the app.
class PropertiesController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY openChanged)
    Q_PROPERTY(QString title READ title NOTIFY openChanged)
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)        // [{title, rows: [{label, value}]}]
    Q_PROPERTY(QString status READ status NOTIFY sectionsChanged)                 // what is still being worked out
    Q_PROPERTY(bool canHash READ canHash NOTIFY sectionsChanged)
    Q_PROPERTY(bool hashing READ hashing NOTIFY hashesChanged)
    Q_PROPERTY(double hashProgress READ hashProgress NOTIFY hashesChanged)
    Q_PROPERTY(QVariantList hashes READ hashes NOTIFY hashesChanged)              // [{label, value}]
    Q_PROPERTY(QString hashError READ hashError NOTIFY hashesChanged)

public:
    explicit PropertiesController(QObject* parent = nullptr);

    bool isOpen() const { return m_open; }
    QString title() const { return m_title; }
    QVariantList sections() const;
    QString status() const { return m_status; }
    bool canHash() const { return m_canHash; }
    bool hashing() const { return m_hashing; }
    double hashProgress() const { return m_hashProgress; }
    QVariantList hashes() const { return m_hashes; }
    QString hashError() const { return m_hashError; }

    Q_INVOKABLE void show(const QStringList& paths);
    Q_INVOKABLE void close();
    Q_INVOKABLE void startHashes();
    Q_INVOKABLE void cancelHashes();
    Q_INVOKABLE void copyText(const QString& text);
    Q_INVOKABLE void copyAll();          // every section as plain text

signals:
    void openChanged();
    void sectionsChanged();
    void hashesChanged();

private:
    void collect(const std::string& path);
    void setSection(PropertySection section);
    void toUi(const BackgroundJob::Handle& handle, quint64 generation, std::function<void()> work);

    BackgroundJob m_job;
    BackgroundJob m_hashJob;
    quint64 m_generation = 0;
    QString m_path;
    bool m_open = false;
    QString m_title;
    std::vector<PropertySection> m_sections;
    QString m_status;
    bool m_canHash = false;
    bool m_hashing = false;
    double m_hashProgress = 0;
    QVariantList m_hashes;
    QString m_hashError;
};
