#include "logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

Logger *Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject *parent) : QObject(parent) {}

Logger::~Logger()
{
    shutdown();
}

void Logger::init(const QString &logPath, int maxFiles, qint64 maxSize)
{
    if (m_initialized) return;

    m_logPath = logPath;
    m_maxFiles = maxFiles;
    m_maxSize = maxSize;

    // 创建日志目录
    QDir dir(QFileInfo(m_logPath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 打开日志文件
    m_logFile.setFileName(m_logPath);
    if (!m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << m_logPath;
        return;
    }

    m_logStream.setDevice(&m_logFile);
    m_initialized = true;

    // 安装消息处理器
    qInstallMessageHandler(Logger::messageHandler);

    // 写入启动日志
    write("INFO", "=== Application Started ===");
    write("INFO", QString("Log file: %1").arg(m_logPath));
}

void Logger::shutdown()
{
    if (!m_initialized) return;

    // 恢复默认消息处理器
    qInstallMessageHandler(nullptr);

    write("INFO", "=== Application Shutdown ===");

    m_logStream.flush();
    m_logFile.close();
    m_initialized = false;
}

void Logger::write(const QString &level, const QString &message,
                   const QString &file, int line, const QString &function)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || !m_logFile.isOpen()) return;

    // 检查是否需要轮转
    rotateLogFile();

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");

    QString logEntry;
    if (!file.isEmpty()) {
        logEntry = QString("[%1] [%2] %3 (%4:%5 - %6)")
                   .arg(timestamp, level, message, file, QString::number(line), function);
    } else {
        logEntry = QString("[%1] [%2] %3")
                   .arg(timestamp, level, message);
    }

    m_logStream << logEntry << Qt::endl;
    m_logStream.flush();
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString level;
    switch (type) {
    case QtDebugMsg: level = "DEBUG"; break;
    case QtInfoMsg:  level = "INFO";  break;
    case QtWarningMsg: level = "WARN";  break;
    case QtCriticalMsg: level = "ERROR"; break;
    case QtFatalMsg:  level = "FATAL"; break;
    default: level = "UNKNOWN"; break;
    }

    Logger::instance()->write(
        level, msg,
        QString(context.file), context.line,
        QString(context.function)
    );

    // 同时输出到控制台
    if (type == QtFatalMsg) {
        fprintf(stderr, "[%s] %s\n", level.toUtf8().constData(), msg.toUtf8().constData());
    } else {
        fprintf(stdout, "[%s] %s\n", level.toUtf8().constData(), msg.toUtf8().constData());
    }
    fflush(stdout);
}

void Logger::rotateLogFile()
{
    if (!m_logFile.isOpen()) return;

    qint64 size = m_logFile.size();
    if (size < m_maxSize) return;

    // 关闭当前文件
    m_logStream.flush();
    m_logFile.close();

    // 重命名旧文件
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupName = QString("%1.%2.log").arg(
        QFileInfo(m_logPath).completeBaseName(),
        timestamp
    );
    QString backupPath = QFileInfo(m_logPath).absolutePath() + "/" + backupName;
    QFile::rename(m_logPath, backupPath);

    // 打开新文件
    if (m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
        write("INFO", "=== Log rotated ===");
    }

    // 清理旧日志
    cleanupOldLogs();
}

void Logger::cleanupOldLogs()
{
    QDir dir(QFileInfo(m_logPath).absolutePath());
    QStringList filters;
    filters << QString("%1.*.log").arg(QFileInfo(m_logPath).completeBaseName());
    filters << QString("%1.log.*").arg(QFileInfo(m_logPath).completeBaseName());

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    // 删除多余的文件（保留最近 m_maxFiles 个）
    for (int i = m_maxFiles; i < files.size(); ++i) {
        QFile::remove(files[i].absoluteFilePath());
        qDebug() << "Removed old log file:" << files[i].fileName();
    }
}
