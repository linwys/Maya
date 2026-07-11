#pragma once
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <bass.h>

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
//mp3_flac extract
    static void parse_id3v2(const char* data, size_t size, Track& track) {
        if (size < 10 || std::strncmp(data, "ID3", 3) != 0) return;
        
        unsigned char major_version = data[3];
        if (major_version != 3 && major_version != 4 && major_version != 2) return;

        size_t idx = 10;
        while (idx + 10 < size) {
            char frame_id[5] = {0};
            size_t frame_size = 0;
            
            if (major_version == 2) {
                if (idx + 6 >= size) break;
                std::memcpy(frame_id, data + idx, 3);
                frame_size = (static_cast<unsigned char>(data[idx + 3]) << 16) |
                             (static_cast<unsigned char>(data[idx + 4]) << 8) |
                              static_cast<unsigned char>(data[idx + 5]);
                idx += 6;
            } else {
                std::memcpy(frame_id, data + idx, 4);
                unsigned char s1 = data[idx + 4];
                unsigned char s2 = data[idx + 5];
                unsigned char s3 = data[idx + 6];
                unsigned char s4 = data[idx + 7];
                if (major_version == 4) {
                    frame_size = (s1 << 21) | (s2 << 14) | (s3 << 7) | s4;
                } else {
                    frame_size = (s1 << 24) | (s2 << 16) | (s3 << 8) | s4;
                }
                idx += 10;
            }

            if (frame_size == 0 || idx + frame_size > size) break;

            auto parse_text_frame = [](const char* frame_data, size_t f_size) -> QString {
                if (f_size <= 1) return "";
                unsigned char encoding = frame_data[0];
                const char* text_ptr = frame_data + 1;
                size_t text_len = f_size - 1;

                if (encoding == 0) {
                    return QString::fromLatin1(text_ptr, static_cast<int>(text_len)).trimmed();
                } else if (encoding == 1) {
                    if (text_len >= 2) {
                        return QString::fromUtf16(reinterpret_cast<const char16_t*>(text_ptr), static_cast<int>(text_len / 2)).trimmed();
                    }
                } else if (encoding == 2) {
                    if (text_len >= 2) {
                        QByteArray swapped(text_ptr, static_cast<int>(text_len));
                        for (int i = 0; i < swapped.size() - 1; i += 2) {
                            std::swap(swapped[i], swapped[i + 1]);
                        }
                        return QString::fromUtf16(reinterpret_cast<const char16_t*>(swapped.constData()), static_cast<int>(swapped.size() / 2)).trimmed();
                    }
                } else if (encoding == 3) {
                    return QString::fromUtf8(text_ptr, static_cast<int>(text_len)).trimmed();
                }
                return "";
            };

            QString f_id = QString::fromLatin1(frame_id);
            if (f_id == "TIT2" || f_id == "TT2") {
                QString t = parse_text_frame(data + idx, frame_size);
                if (!t.isEmpty()) track.title = t;
            } else if (f_id == "TPE1" || f_id == "TP1") {
                QString a = parse_text_frame(data + idx, frame_size);
                if (!a.isEmpty()) track.artist = a;
            } else if (f_id == "TALB" || f_id == "TAL") {
                QString al = parse_text_frame(data + idx, frame_size);
                if (!al.isEmpty()) track.album = al;
            }

            idx += frame_size;
        }
    }

    static Track extract_meta(const QString& file_path) {
        Track track;
        track.id = QUuid::createUuid();
        track.file_path = file_path;
        track.is_local = true;

        QFileInfo file_info(file_path);
        track.title = file_info.completeBaseName();
        track.artist = "Unknown Artist";
        track.album = "Unknown Album";

        QString native_path = QDir::toNativeSeparators(file_path);
#if defined(Q_OS_WIN)
        const void* file_ptr = native_path.utf16();
#else
        QByteArray path_utf8 = native_path.toUtf8();
        const void* file_ptr = path_utf8.constData();
#endif

        unsigned long temp_stream = BASS_StreamCreateFile(FALSE, file_ptr, 0, 0, BASS_STREAM_DECODE | BASS_UNICODE);
        if (temp_stream) {
            double secs = BASS_ChannelBytes2Seconds(temp_stream, BASS_ChannelGetLength(temp_stream, BASS_POS_BYTE));
            track.duration = std::chrono::seconds(static_cast<int64_t>(secs));

            if (const char* ogg_tags = BASS_ChannelGetTags(temp_stream, BASS_TAG_OGG)) {
                while (*ogg_tags) {
                    QString line = QString::fromUtf8(ogg_tags);
                    int eq_idx = line.indexOf('=');
                    if (eq_idx != -1) {
                        QString key = line.left(eq_idx).toUpper().trimmed();
                        QString val = line.mid(eq_idx + 1).trimmed();
                        if (key == "TITLE") track.title = val;
                        else if (key == "ARTIST") track.artist = val;
                        else if (key == "ALBUM") track.album = val;
                    }
                    ogg_tags += std::strlen(ogg_tags) + 1;
                }
            } else if (const char* id3v2_tags = BASS_ChannelGetTags(temp_stream, BASS_TAG_ID3V2)) {
                DWORD len = BASS_ChannelGetTags(temp_stream, BASS_TAG_ID3V2_BINARY) ? 0 : 1024 * 1024;
                parse_id3v2(id3v2_tags, len, track);
            } else if (const char* id3_tags = BASS_ChannelGetTags(temp_stream, BASS_TAG_ID3)) {
                struct TAG_ID3 {
                    char id[3];
                    char title[30];
                    char artist[30];
                    char album[30];
                    char year[4];
                    char comment[30];
                    unsigned char genre;
                };
                const TAG_ID3* id3 = reinterpret_cast<const TAG_ID3*>(id3_tags);
                track.title = QString::fromLatin1(id3->title, 30).trimmed();
                track.artist = QString::fromLatin1(id3->artist, 30).trimmed();
                track.album = QString::fromLatin1(id3->album, 30).trimmed();
            }

            BASS_StreamFree(temp_stream);
        }

        return track;
    }
}; //track

}
