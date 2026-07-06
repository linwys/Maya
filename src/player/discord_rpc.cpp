#include "discord_rpc.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

#if !defined(Q_OS_WIN)
#include <unistd.h>
#endif

namespace player {

static QString extract_youtube_thumbnail(const QString& url) {
    static QRegularExpression rx(R"((?:v=|\/v\/|embed\/|youtu\.be\/|\/shorts\/)([a-zA-Z0-9_-]{11}))");
    QRegularExpressionMatch match = rx.match(url);
    if (match.hasMatch()) {
        return QString("https://img.youtube.com/vi/%1/hqdefault.jpg").arg(match.captured(1));
    }
    return "";
}

DiscordRPC::DiscordRPC(QObject* parent) : QObject(parent), m_socket(new QLocalSocket(this)) {
    connect(m_socket, &QLocalSocket::connected, this, [this]() {
        m_connected = true;
        QJsonObject handshake;
        handshake["v"] = 1;
        handshake["client_id"] = m_client_id;
        send_packet(0, handshake);
    });

    connect(m_socket, &QLocalSocket::disconnected, this, [this]() {
        m_connected = false;
    });
}

DiscordRPC::~DiscordRPC() {
    shutdown();
}

void DiscordRPC::initialize(const QString& client_id) {
    m_client_id = client_id;
    connect_to_discord();
}

void DiscordRPC::connect_to_discord() {
    if (m_connected) return;
    
#if defined(Q_OS_WIN)
    m_socket->connectToServer(R"(\\.\pipe\discord-ipc-0)");
#else
    QString path = "/run/user/" + QString::number(getuid()) + "/discord-ipc-0";
    if (!QFile::exists(path)) {
        path = "/tmp/discord-ipc-0";
    }
    m_socket->connectToServer(path);
#endif
}

void DiscordRPC::send_packet(uint32_t opcode, const QJsonObject& payload) {
    if (m_socket->state() != QLocalSocket::ConnectedState) return;

    QByteArray json_bytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    uint32_t length = static_cast<uint32_t>(json_bytes.size());

    QByteArray packet;
    packet.append(reinterpret_cast<const char*>(&opcode), sizeof(opcode));
    packet.append(reinterpret_cast<const char*>(&length), sizeof(length));
    packet.append(json_bytes);

    m_socket->write(packet);
    m_socket->flush();
}

void DiscordRPC::update_presence(const Track& track, int64_t elapsed_seconds) {
    if (!m_connected) {
        connect_to_discord();
        if (!m_connected) return;
    }

    QJsonObject activity;
    activity["details"] = track.title;
    activity["state"] = "by " + track.artist;

    QJsonObject assets;
    QString img_url = track.thumbnail_url;
    if (img_url.isEmpty()) {
        img_url = extract_youtube_thumbnail(track.source_url);
    }
    if (img_url.isEmpty() && track.artwork_path.startsWith("http")) {
        img_url = track.artwork_path;
    }

    if (!img_url.isEmpty()) {
        assets["large_image"] = img_url;
    } else {
        assets["large_image"] = "logo";
    }
    assets["large_text"] = "by linwys";

    activity["assets"] = assets;

    QJsonObject timestamps;
    int64_t now = QDateTime::currentSecsSinceEpoch();
    timestamps["start"] = now - elapsed_seconds;
    timestamps["end"] = now - elapsed_seconds + track.duration.count();
    activity["timestamps"] = timestamps;

    QJsonObject args;
    args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
    args["activity"] = activity;

    QJsonObject root;
    root["cmd"] = "SET_ACTIVITY";
    root["args"] = args;
    root["nonce"] = "Maya_RPC";

    send_packet(1, root);
}

void DiscordRPC::clear_presence() {
    if (!m_connected) return;

    QJsonObject args;
    args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
    args["activity"] = QJsonValue(QJsonValue::Null);

    QJsonObject root;
    root["cmd"] = "SET_ACTIVITY";
    root["args"] = args;
    root["nonce"] = "Maya_RPC_Clear";

    send_packet(1, root);
}

void DiscordRPC::shutdown() { // 0001
    clear_presence();
        if (m_socket) {
        m_socket->close();
    }
    m_connected = false;
}

}