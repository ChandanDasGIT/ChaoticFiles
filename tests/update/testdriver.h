#pragma once
#include <QFile>
#include <QObject>
#include <QTimer>

class QQmlApplicationEngine;

// For tests/update only, and never in a release. Reads commands from the file in CF_TEST_COMMANDS and answers in the
// file in CF_TEST_ANSWERS, so a test can press the real buttons of the real window and look at what it shows.
class TestDriver : public QObject {
    Q_OBJECT

public:
    explicit TestDriver(QQmlApplicationEngine* engine);

private:
    void poll();
    void run(const QByteArray& line);

    QQmlApplicationEngine* m_engine;
    QTimer m_timer;
    QByteArray m_read;
    qint64 m_position = 0;
};
