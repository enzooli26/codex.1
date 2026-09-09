#pragma once
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QDebug>
class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger* instance();
    void init(const QString &logPath, int maxFiles = 10, qint64 maxSize = 1024 * 1024 * 10); // 10MB
    void shutdown();

    void write(const QString &level, const QString &message,
               const QString &file = QString(), int line = 0,
               const QString &function = QString());

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    static void messageHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &msg);

    void rotateLogFile();
    void cleanupOldLogs();

    QFile m_logFile;
    QTextStream m_logStream;
    QMutex m_mutex;

    QString m_logPath;
    int m_maxFiles;
    qint64 m_maxSize;
    bool m_initialized = false;

    static Logger *m_instance;
};
