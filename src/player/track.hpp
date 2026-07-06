#pragma once
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <chrono>

namespace player {

struct Track {
    QUuid id;
    QString file_path;
    QString title;
    QString artist;
    QString album;
    std::chrono::seconds duration{0};
    QString source_url;
    QString artwork_path;
    QString thumbnail_url;
    bool is_favorite{false};
    bool is_local{true};

    QJsonObject to_json() const {
        QJsonObject json;
        json["id"] = id.toString();
        json["file_path"] = file_path;
        json["title"] = title;
        json["artist"] = artist;
        json["album"] = album;
        json["duration"] = static_cast<double>(duration.count());
        json["source_url"] = source_url;
        json["artwork_path"] = artwork_path;
        json["thumbnail_url"] = thumbnail_url;
        json["is_favorite"] = is_favorite;
        json["is_local"] = is_local;
        return json;
    }

    static Track from_json(const QJsonObject& json) {
        Track track;
        track.id = QUuid::fromString(json["id"].toString());
        track.file_path = json["file_path"].toString();
        track.title = json["title"].toString();
        track.artist = json["artist"].toString();
        track.album = json["album"].toString();
        track.duration = std::chrono::seconds(static_cast<int64_t>(json["duration"].toDouble()));
        track.source_url = json["source_url"].toString();
        track.artwork_path = json["artwork_path"].toString();
        track.thumbnail_url = json["thumbnail_url"].toString();
        track.is_favorite = json["is_favorite"].toBool();
        track.is_local = json["is_local"].toBool();
        return track;
    }
};

}