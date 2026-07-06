#pragma once
#include <QObject>
#include <QTimer>
#include <vector>
#include "track.hpp"

namespace player {

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

enum class RepeatMode {
    Off,
    All,
    One
};

class AudioPipeline : public QObject {
    Q_OBJECT
public:
    explicit AudioPipeline(QObject* parent = nullptr);
    ~AudioPipeline() override;

    void initialize();
    void play(const Track& track);
    void play_queue(const std::vector<Track>& queue, size_t start_index);
    void pause();
    void resume();
    void stop();
    void seek(int seconds);
    void set_volume(float volume);
    void set_normalize(bool enabled);

    void next();
    void prev();
    void set_shuffle(bool enabled);
    void set_repeat(RepeatMode mode);

    PlaybackState state() const { return m_state; }
    Track current_track() const { return m_current_track; }
    std::chrono::seconds current_position() const;
    unsigned long bass_stream() const { return m_bass_stream; }

signals:
    void track_changed(const Track& track);
    void state_changed(PlaybackState new_state);
    void position_updated(std::chrono::seconds seconds);

private:
    static unsigned long __stdcall bass_stream_callback(unsigned long handle, void* buffer, unsigned long length, void* user);
    static void __stdcall end_sync_proc(unsigned long handle, unsigned long channel, unsigned long data, void* user);

    void feed_bass(void* buffer, unsigned long length);
    Q_INVOKABLE void handle_track_ended();

    PlaybackState m_state{PlaybackState::Stopped};
    Track m_current_track;
    unsigned long m_bass_stream{0};
    float m_volume{0.8f};
    QTimer* m_timer{nullptr};
    unsigned long m_fx_normalize{0};
    bool m_normalize_enabled{false};

    std::vector<Track> m_queue;
    size_t m_current_index{0};
    bool m_shuffle{false};
    RepeatMode m_repeat{RepeatMode::Off};
    std::atomic<int64_t> m_last_emitted_sec{-1};
};

}