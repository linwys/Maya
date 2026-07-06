#pragma once
#include <QObject>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <vector>
#include <map>
#include "track.hpp"

namespace player {

struct Playlist {
    QString name;
    QString cover_path;
    std::vector<QUuid> track_ids;

    QJsonObject to_json() const {
        QJsonObject json;
        json["name"] = name;
        json["cover_path"] = cover_path;
        QJsonArray arr;
        for (const auto& id : track_ids) {
            arr.append(id.toString());
        }
        json["track_ids"] = arr;
        return json;
    }

    static Playlist from_json(const QJsonObject& json) {
        Playlist pl;
        pl.name = json["name"].toString();
        pl.cover_path = json["cover_path"].toString();
        QJsonArray arr = json["track_ids"].toArray();
        for (const auto& val : arr) {
            pl.track_ids.push_back(QUuid::fromString(val.toString()));
        }
        return pl;
    }
};

class Db : public QObject {
    Q_OBJECT
public:
    explicit Db(QObject* parent = nullptr);
    ~Db() override;

    void load();
    void save();

    const std::vector<Track>& tracks() const { return m_tracks; }
    void add_track(const Track& track);         // +
    void remove_track(const QUuid& id); // +
    void toggle_favorite(const QUuid& id);      // +
    void update_track(const QUuid& id, const QString& title, const QString& artist, const QString& album, const QString& artwork_path); // +

    const std::map<QString, Playlist>& playlists() const { return m_playlists; }    // +
    void create_playlist(const QString& name, const QString& cover_path = "");  // +
    void remove_playlist(const QString& name);  // +
    void add_to_playlist(const QString& playlist_name, const QUuid& track_id);  // +
    void add_to_playlist_batch(const QString& playlist_name, const std::vector<QUuid>& track_ids);  // +
    void remove_from_playlist(const QString& playlist_name, const QUuid& track_id); // +
    void clear_playlist_tracks(const QString& name);    // +
    void update_playlist_cover(const QString& name, const QString& cover_path); // +
    void update_playlist_data(const QString& name, const QString& cover_path, const std::vector<QUuid>& track_ids); // +

    QString save_format() const { return m_save_format; }   // +
    bool crossfade() const { return m_crossfade; }  // +    
    void set_crossfade(bool enabled) { m_crossfade = enabled; queue_save(); }   // +
    bool gapless_playback() const { return m_gapless_playback; }    // +
    void set_gapless_playback(bool enabled) { m_gapless_playback = enabled; queue_save(); } // +
    bool normalize_volume() const { return m_normalize_volume; }    // +
    void set_normalize_volume(bool enabled) { m_normalize_volume = enabled; queue_save(); } // +
    bool auto_download_favorites() const { return m_auto_download_favorites; }  // +
    void set_auto_download_favorites(bool enabled) { m_auto_download_favorites = enabled; queue_save(); }   // +
    bool wifi_only() const { return m_wifi_only; }  // + @note: not may work
    void set_wifi_only(bool enabled) { m_wifi_only = enabled; queue_save(); }   // + @note: not may work
    void set_save_format(const QString& format) { m_save_format = format; queue_save(); } // + 
    bool equalizer_enabled() const { return m_equalizer_enabled; } // + 
    void set_equalizer_enabled(bool enabled) { m_equalizer_enabled = enabled; queue_save(); } // + 
    bool discord_rpc_enabled() const { return m_discord_rpc_enabled; } // + 
    void set_discord_rpc_enabled(bool enabled) { m_discord_rpc_enabled = enabled; queue_save(); } // + 
    float volume() const { return m_volume; } // + 
    void set_volume(float vol) { m_volume = vol; queue_save(); } // + 
    QString storage_dir() const;     // + 
    void change_storage_dir(const QString& new_dir, bool copy_existing); // + 

signals:
    void library_changed(); // + 
    void playlists_changed(); // + 
    void track_favorite_changed(const QUuid& id, bool is_favorite); // + 

private:
    QString storage_path() const;
    void update_favourites_playlist_state();
    void queue_save();

    std::vector<Track> m_tracks;
    std::map<QString, Playlist> m_playlists;
    QString m_save_format{"flac"};
    bool m_crossfade{true};
    bool m_gapless_playback{true};
    bool m_normalize_volume{false};
    bool m_auto_download_favorites{true};
    bool m_wifi_only{false};
    bool m_equalizer_enabled{true};
    bool m_discord_rpc_enabled{false};
    float m_volume{0.8f};
    QTimer* m_save_timer{nullptr};
};

}