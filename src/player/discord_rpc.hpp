// player/discord_rpc.hpp
#pragma once
#include <QObject>
#include <QLocalSocket>
#include "track.hpp"

namespace player {

class DiscordRPC : public QObject {
    Q_OBJECT
public:
    explicit DiscordRPC(QObject* parent = nullptr);
    ~DiscordRPC() override;

    void initialize(const QString& client_id); // + 
    void update_presence(const Track& track, int64_t elapsed_seconds); // + 
    void clear_presence();    // + 
    void shutdown(); // +  @note : disconnect socket

private:
    void connect_to_discord(); // + 
    void send_packet(uint32_t opcode, const QJsonObject& payload); // + 

    QLocalSocket* m_socket{nullptr};
    QString m_client_id;
    bool m_connected{false};
};

}