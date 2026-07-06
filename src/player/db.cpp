#include "db.hpp"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace player {

Db::Db(QObject* parent) : QObject(parent) {
    m_save_timer = new QTimer(this);
    m_save_timer->setSingleShot(true);
    m_save_timer->setInterval(500);//write
    connect(m_save_timer, &QTimer::timeout, this, [this]() {
        save();
    });

    load();
}

Db::~Db() {
    if (m_save_timer && m_save_timer->isActive()) {
        m_save_timer->stop();
        save();
    }
}

QString Db::storage_dir() const {//ssss0t
    QSettings settings("linwys", "maya");
    QString app_data_default = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return settings.value("storage_dir", app_data_default).toString();
}

QString Db::storage_path() const {
    QDir().mkpath(storage_dir());
    return QDir(storage_dir()).filePath("library.json");
}

void Db::load() {
    QFile file(storage_path());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    m_save_format = root["save_format"].toString("flac");
    m_crossfade = root["crossfade"].toBool(true);
    m_gapless_playback = root["gapless_playback"].toBool(true);
    m_normalize_volume = root["normalize_volume"].toBool(false);
    m_auto_download_favorites = root["auto_download_favorites"].toBool(true);
    m_wifi_only = root["wifi_only"].toBool(false);
    m_equalizer_enabled = root["equalizer_enabled"].toBool(true);
    m_discord_rpc_enabled = root["discord_rpc_enabled"].toBool(false);
    m_volume = static_cast<float>(root["volume"].toDouble(0.8));
    
    m_tracks.clear();
    QJsonArray tracks_arr = root["tracks"].toArray();
    for (const auto& val : tracks_arr) {
        m_tracks.push_back(Track::from_json(val.toObject()));
    }

    m_playlists.clear();
    QJsonObject playlists_obj = root["playlists_new"].toObject();
    if (playlists_obj.isEmpty()) {
        QJsonObject old_obj = root["playlists"].toObject();
        for (auto it = old_obj.begin(); it != old_obj.end(); ++it) {
            Playlist pl;
            pl.name = it.key();
            QJsonArray arr = it.value().toArray();
            for (const auto& val : arr) {
                pl.track_ids.push_back(QUuid::fromString(val.toString()));
            }
            m_playlists[pl.name] = pl;
        }
    } else {
        for (auto it = playlists_obj.begin(); it != playlists_obj.end(); ++it) {
            m_playlists[it.key()] = Playlist::from_json(it.value().toObject());
        }
    }

    QHash<QUuid, bool> track_exists;
    track_exists.reserve(static_cast<qsizetype>(m_tracks.size()));
    for (const auto& track : m_tracks) {
        track_exists.insert(track.id, true);
    }

    bool modified = false;
    for (auto& [name, pl] : m_playlists) {
        size_t original_size = pl.track_ids.size();
        std::erase_if(pl.track_ids, [&track_exists](const QUuid& id) {
            return !track_exists.contains(id);
        });
        if (pl.track_ids.size() != original_size) {
            modified = true;
        }
    }

    update_favourites_playlist_state();

    if (modified) {
        save();
    }
}

void Db::save() {
    QFile file(storage_path());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonObject root;
    root["save_format"] = m_save_format;
    root["crossfade"] = m_crossfade;
    root["gapless_playback"] = m_gapless_playback;
    root["normalize_volume"] = m_normalize_volume;
    root["auto_download_favorites"] = m_auto_download_favorites;
    root["wifi_only"] = m_wifi_only;
    root["equalizer_enabled"] = m_equalizer_enabled;
    root["discord_rpc_enabled"] = m_discord_rpc_enabled;
    root["volume"] = static_cast<double>(m_volume);
    
    QJsonArray tracks_arr;
    for (const auto& track : m_tracks) {
        tracks_arr.append(track.to_json());
    }
    root["tracks"] = tracks_arr;

    QJsonObject playlists_obj;
    for (const auto& [name, pl] : m_playlists) {
        playlists_obj[name] = pl.to_json();
    }
    root["playlists_new"] = playlists_obj;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void Db::add_track(const Track& track) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [&track](const Track& t) {
        return t.file_path == track.file_path;
    });
    if (it == m_tracks.end()) {
        m_tracks.push_back(track);
        queue_save();
        emit library_changed();
    }
}

void Db::remove_track(const QUuid& id) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [&id](const Track& t) {
        return t.id == id;
    });
    if (it != m_tracks.end()) {
        m_tracks.erase(it);
        for (auto& [name, pl] : m_playlists) {
            std::erase(pl.track_ids, id);
        }
        update_favourites_playlist_state();
        queue_save();
        emit library_changed();
        emit playlists_changed();
    }
}

void Db::toggle_favorite(const QUuid& id) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [&id](const Track& t) {
        return t.id == id;
    });
    if (it != m_tracks.end()) {
        it->is_favorite = !it->is_favorite;

        update_favourites_playlist_state();
        queue_save();
        emit playlists_changed();
        emit track_favorite_changed(id, it->is_favorite);
    }
}

void Db::update_track(const QUuid& id, const QString& title, const QString& artist, const QString& album, const QString& artwork_path) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [&id](const Track& t) {
        return t.id == id;
    });
    if (it != m_tracks.end()) {
        it->title = title;
        it->artist = artist;
        it->album = album;
        if (!artwork_path.isEmpty()) {
            it->artwork_path = artwork_path;
        }
        save();
        emit library_changed();
    }
}

void Db::create_playlist(const QString& name, const QString& cover_path) {
    if (!m_playlists.contains(name)) {
        Playlist pl;
        pl.name = name;
        pl.cover_path = cover_path;
        m_playlists[name] = pl;
        save();
        emit playlists_changed();
    }
}

void Db::remove_playlist(const QString& name) {
    if (m_playlists.erase(name) > 0) {
        save();
        emit playlists_changed();
    }
}

void Db::add_to_playlist(const QString& playlist_name, const QUuid& track_id) {
    if (m_playlists.contains(playlist_name)) {
        auto& pl = m_playlists[playlist_name];
        if (std::find(pl.track_ids.begin(), pl.track_ids.end(), track_id) == pl.track_ids.end()) {
            pl.track_ids.push_back(track_id);
            save();
            emit playlists_changed();
        }
    }
}

void Db::add_to_playlist_batch(const QString& playlist_name, const std::vector<QUuid>& track_ids) {
    if (m_playlists.contains(playlist_name)) {
        auto& pl = m_playlists[playlist_name];
        bool modified = false;
        for (const auto& id : track_ids) {
            if (std::find(pl.track_ids.begin(), pl.track_ids.end(), id) == pl.track_ids.end()) {
                pl.track_ids.push_back(id);
                modified = true;
            }
        }
        if (modified) {
            save();
            emit playlists_changed();
        }
    }
}

void Db::remove_from_playlist(const QString& playlist_name, const QUuid& track_id) {
    if (m_playlists.contains(playlist_name)) {
        auto& pl = m_playlists[playlist_name];
        auto it = std::find(pl.track_ids.begin(), pl.track_ids.end(), track_id);
        if (it != pl.track_ids.end()) {
            pl.track_ids.erase(it);
            save();
            emit playlists_changed();
        }
    }
}

void Db::clear_playlist_tracks(const QString& name) {
    if (m_playlists.contains(name)) {
        m_playlists[name].track_ids.clear();
        save();
        emit playlists_changed();
    }
}

void Db::update_playlist_cover(const QString& name, const QString& cover_path) {
    if (m_playlists.contains(name)) {
        m_playlists[name].cover_path = cover_path;
        save();
        emit playlists_changed();
    }
}

void Db::update_playlist_data(const QString& name, const QString& cover_path, const std::vector<QUuid>& track_ids) {
    if (m_playlists.contains(name)) {
        auto& pl = m_playlists[name];
        pl.cover_path = cover_path;
        pl.track_ids = track_ids;
        queue_save();
        emit playlists_changed();
    }
}

void Db::update_favourites_playlist_state() {
    std::vector<QUuid> fav_ids;
    for (const auto& track : m_tracks) {
        if (track.is_favorite) {
            fav_ids.push_back(track.id);
        }
    }

    const QString fav_name = "Favourites";
    if (!fav_ids.empty()) {
        if (!m_playlists.contains(fav_name)) {
            Playlist pl;
            pl.name = fav_name;
            m_playlists[fav_name] = pl;
        }
        m_playlists[fav_name].track_ids = fav_ids;
    } else {
        if (m_playlists.contains(fav_name)) {
            m_playlists.erase(fav_name);
        }
    }
}

void Db::queue_save() {
    m_save_timer->start();
}

void Db::change_storage_dir(const QString& new_dir, bool copy_existing) {
    QString old_path = storage_path();
    QString new_path = QDir(new_dir).filePath("library.json");

    if (copy_existing && QFile::exists(old_path)) {
        QFile::remove(new_path);
        QFile::copy(old_path, new_path);
    }

    QSettings settings("linwys", "maya");
    settings.setValue("storage_dir", new_dir);

    load();
    emit library_changed();
    emit playlists_changed();
}

}
