#pragma once
#include <QObject>
#include <QString>
#include <QProcess>
#include <vector>
#include <functional>
#include "track.hpp"

namespace player {

struct WebTrack {
    QString id;
    QString title;
    QString artist;
    std::chrono::seconds duration{0};
    QString source_url;
    QString thumbnail_url;
};

enum class DlState {
    Queued,
    Downloading,
    Completed,
    Failed
};

struct DlTask {
    WebTrack track;
    QString output_dir;
    QString format;
    std::function<void(bool, QString, Track)> callback;
    DlState state{DlState::Queued};
    QString error_message;
};

class DlMgr : public QObject {
    Q_OBJECT
public:
    explicit DlMgr(QObject* parent = nullptr);
    ~DlMgr() override = default;  // +

    void search_online(const QString& query, std::function<void(std::vector<WebTrack>, QString)> callback); // +
    void parse_online_playlist(const QString& url, std::function<void(std::vector<WebTrack>, QString)> callback); // +
    void cancel_search();   // +
    
    void download(const WebTrack& track, const QString& output_dir, const QString& format, std::function<void(bool, QString, Track)> callback); // +
    void retry_download(const QString& track_id);   // +

signals: 
    void progress_changed(const QString& track_id, double percentage);  // +
    void download_queued(const player::WebTrack& track);    // +
    void download_started(const QString& track_id); // +
    void download_completed(const QString& track_id, const player::Track& downloaded);  // +
    void download_failed(const QString& track_id, const QString& error_message);    // +

private:
    void process_next_in_queue();          // +
    void start_download_process(DlTask& item);  // +

    QProcess* m_search_process{nullptr};
    QProcess* m_active_download_process{nullptr};
    std::vector<DlTask> m_queue;
    bool m_is_downloading{false};
};

}