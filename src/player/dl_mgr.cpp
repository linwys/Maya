#include "dl_mgr.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>

namespace player {

DlMgr::DlMgr(QObject* parent) : QObject(parent) {}

void DlMgr::search_online(const QString& query, std::function<void(std::vector<WebTrack>, QString)> callback) {
    cancel_search();

    m_search_process = new QProcess(this);
    QString app_dir = QCoreApplication::applicationDirPath();
    QString yt_dlp_path = QDir(app_dir).filePath("yt-dlp");
#if defined(Q_OS_WIN)
    yt_dlp_path += ".exe";
#endif

    QStringList arguments;
    arguments << QString("ytsearch5:%1").arg(query)
              << "--dump-json"
              << "--no-playlist";

    connect(m_search_process, &QProcess::finished, this, [this, callback](int exit_code) {
        std::vector<WebTrack> results;
        QString err;
        if (exit_code == 0 && m_search_process) {
            QString output = QString::fromUtf8(m_search_process->readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            for (const auto& line : lines) {
                QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    WebTrack track;
                    track.id = obj["id"].toString();
                    track.title = obj["title"].toString();
                    track.artist = obj["uploader"].toString();
                    track.duration = std::chrono::seconds(static_cast<int64_t>(obj["duration"].toDouble()));
                    track.source_url = obj["webpage_url"].toString();
                    track.thumbnail_url = obj["thumbnail"].toString();
                    results.push_back(track);
                }
            }
        } else if (m_search_process) {
            err = QString::fromUtf8(m_search_process->readAllStandardError()).trimmed();
            if (err.isEmpty()) {
                err = "Network block, geo-restriction, or yt-dlp timeout";
            } else {
                static QRegularExpression err_rx("ERROR: (.*)");
                QRegularExpressionMatch m = err_rx.match(err);
                if (m.hasMatch()) {
                    err = m.captured(1);
                }
            }
        }
        callback(results, err);
        if (m_search_process) {
            m_search_process->deleteLater();
            m_search_process = nullptr;
        }
    });

    m_search_process->start(yt_dlp_path, arguments);
}

void DlMgr::parse_online_playlist(const QString& url, std::function<void(std::vector<WebTrack>, QString)> callback) {
    cancel_search();

    m_search_process = new QProcess(this);
    QString app_dir = QCoreApplication::applicationDirPath();
    QString yt_dlp_path = QDir(app_dir).filePath("yt-dlp");
#if defined(Q_OS_WIN)
    yt_dlp_path += ".exe";
#endif

    QStringList arguments;
    arguments << "--flat-playlist"
              << "--dump-json"
              << url;

    connect(m_search_process, &QProcess::finished, this, [this, callback](int exit_code) {
        std::vector<WebTrack> results;
        QString err;
        if (exit_code == 0 && m_search_process) {
            QString output = QString::fromUtf8(m_search_process->readAllStandardOutput());
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            for (const auto& line : lines) {
                QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    WebTrack track;
                    track.id = obj["id"].toString();
                    track.title = obj["title"].toString();
                    track.artist = obj["uploader"].toString() == "" ? obj["artist"].toString() : obj["uploader"].toString();
                    track.duration = std::chrono::seconds(static_cast<int64_t>(obj["duration"].toDouble()));
                    track.source_url = obj["url"].toString().isEmpty() ? obj["webpage_url"].toString() : obj["url"].toString();
                    track.thumbnail_url = obj["thumbnail"].toString();
                    results.push_back(track);
                }
            }
        } else if (m_search_process) {
            err = QString::fromUtf8(m_search_process->readAllStandardError()).trimmed();
            if (err.isEmpty()) {
                err = "Network block, geo-restriction, or yt-dlp timeout";
            } else {
                static QRegularExpression err_rx("ERROR: (.*)");
                QRegularExpressionMatch m = err_rx.match(err);
                if (m.hasMatch()) {
                    err = m.captured(1);
                }
            }
        }
        callback(results, err);
        if (m_search_process) {
            m_search_process->deleteLater();
            m_search_process = nullptr;
        }
    });

    m_search_process->start(yt_dlp_path, arguments);
}

void DlMgr::cancel_search() {
    if (m_search_process) {
        if (m_search_process->state() != QProcess::NotRunning) {
            m_search_process->kill();
        }
        m_search_process->deleteLater();
        m_search_process = nullptr;
    }
}

void DlMgr::download(
    const WebTrack& track, 
    const QString& output_dir, 
    const QString& format, 
    std::function<void(bool, QString, Track)> callback
) {
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&track](const DlTask& item) {
        return item.track.id == track.id;
    });
    if (it != m_queue.end()) {
        if (it->state == DlState::Failed) {
            it->state = DlState::Queued;
            it->error_message = "";
            emit download_queued(track);
            process_next_in_queue();
        }
        return;
    }

    m_queue.push_back({track, output_dir, format, callback, DlState::Queued, ""});
    emit download_queued(track);
    process_next_in_queue();
}

void DlMgr::retry_download(const QString& track_id) {
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&track_id](const DlTask& item) {
        return item.track.id == track_id;
    });
    if (it != m_queue.end() && it->state == DlState::Failed) {
        it->state = DlState::Queued;
        it->error_message = "";
        emit download_queued(it->track);
        process_next_in_queue();
    }
}

void DlMgr::process_next_in_queue() {
    if (m_is_downloading) return;

    auto it = std::find_if(m_queue.begin(), m_queue.end(), [](const DlTask& item) {
        return item.state == DlState::Queued;
    });

    if (it == m_queue.end()) {
        return;
    }

    m_is_downloading = true;
    it->state = DlState::Downloading;
    emit download_started(it->track.id);

    start_download_process(*it);
}

void DlMgr::start_download_process(DlTask& item) {
    m_active_download_process = new QProcess(this);
    QString app_dir = QCoreApplication::applicationDirPath();
    QString yt_dlp_path = QDir(app_dir).filePath("yt-dlp");
#if defined(Q_OS_WIN)
    yt_dlp_path += ".exe";
#endif

    QString ext = "flac";
    if (item.format.toLower() == "mp3") {
        ext = "mp3";
    } else if (item.format.toLower() == "opus") {
        ext = "opus";
    }

    QDir().mkpath(item.output_dir);
    QString filename_template = QString("%1/%2 - %3.%(ext)s")
        .arg(item.output_dir)
        .arg(item.track.artist.isEmpty() ? "Unknown Artist" : item.track.artist)
        .arg(item.track.title);

    QStringList arguments;
    arguments << "-f" << "bestaudio/best"
              << item.track.source_url
              << "-x"
              << "--audio-format" << ext
              << "-o" << filename_template
              << "--write-thumbnail"
              << "--convert-thumbnails" << "png"
              << "--embed-thumbnail"
              << "--add-metadata"
              << "--ffmpeg-location" << app_dir;

    QString track_id = item.track.id;

    connect(m_active_download_process, &QProcess::readyReadStandardOutput, this, [this, track_id]() {
        if (!m_active_download_process) return;
        QString output = QString::fromUtf8(m_active_download_process->readAllStandardOutput());
        static QRegularExpression rx(R"(\[\s*download\s*\]\s*(\d+(\.\d+)?)%)");
        QRegularExpressionMatch match = rx.match(output);
        if (match.hasMatch()) {
            double percent = match.captured(1).toDouble();
            emit progress_changed(track_id, percent);
        }
    });

    connect(m_active_download_process, &QProcess::finished, this, [this, track_id](int exit_code) {
        auto it = std::find_if(m_queue.begin(), m_queue.end(), [&track_id](const DlTask& q_item) {
            return q_item.track.id == track_id;
        });

        if (it != m_queue.end()) {
            if (exit_code == 0) {
                it->state = DlState::Completed;
                
                Track downloaded;
                downloaded.id = QUuid::createUuid();
                
                QString ext_str = "flac";
                if (it->format.toLower() == "mp3") ext_str = "mp3";
                else if (it->format.toLower() == "opus") ext_str = "opus";

                QString expected_path = QString("%1/%2 - %3.%4")
                    .arg(it->output_dir)
                    .arg(it->track.artist.isEmpty() ? "Unknown Artist" : it->track.artist)
                    .arg(it->track.title)
                    .arg(ext_str);

                downloaded.file_path = QDir(expected_path).absolutePath();
                downloaded.title = it->track.title;
                downloaded.artist = it->track.artist;
                downloaded.album = "None";
                downloaded.duration = it->track.duration;
                downloaded.source_url = it->track.source_url;
                downloaded.artwork_path = downloaded.file_path;
                downloaded.thumbnail_url = it->track.thumbnail_url;
                downloaded.is_favorite = false;
                downloaded.is_local = true;

                if (it->callback) {
                    it->callback(true, "", downloaded);
                }
                emit download_completed(track_id, downloaded);
            } else {
                it->state = DlState::Failed;
                QString err = QString::fromUtf8(m_active_download_process->readAllStandardError()).trimmed();
                if (err.isEmpty()) {
                    err = "Connection/Proxy error";
                } else {
                    static QRegularExpression err_rx("ERROR: (.*)");
                    QRegularExpressionMatch m = err_rx.match(err);
                    if (m.hasMatch()) {
                        err = m.captured(1);
                    }
                }
                if (err.length() > 50) {
                    err = err.left(47) + "...";
                }
                it->error_message = err;

                if (it->callback) {
                    it->callback(false, err, Track{});
                }
                emit download_failed(track_id, err);
            }
        }

        m_active_download_process->deleteLater();
        m_active_download_process = nullptr;
        m_is_downloading = false;
        process_next_in_queue();
    });

    m_active_download_process->start(yt_dlp_path, arguments);
}

}