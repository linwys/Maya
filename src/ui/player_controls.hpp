#pragma once
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "player/track.hpp"
#include "player/audio_pipeline.hpp"
#include "equalizer_visualizer.hpp"

namespace ui {

class ClickableSlider : public QSlider {
    Q_OBJECT
signals:
    void jumpRequested(int seconds);

public:
    using QSlider::QSlider;

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

class PlayerControls : public QWidget {
    Q_OBJECT
public:
    explicit PlayerControls(QWidget* parent = nullptr);
    ~PlayerControls() override = default;

    void update_track_info(const player::Track& track);
    void update_position(int seconds, int total_seconds);
    void set_playing_state(bool playing);
    void set_visualizer_stream(unsigned long bass_stream);
    void set_volume_value(int val);

signals:
    void play_clicked();
    void pause_clicked();
    void prev_clicked();
    void next_clicked();
    void seek_requested(int seconds);
    void volume_changed(float volume);
    void shuffle_toggled(bool enabled);
    void repeat_changed(player::RepeatMode mode);

private:
    QLabel* m_cover_label{nullptr};
    QLabel* m_title_label{nullptr};
    QLabel* m_artist_label{nullptr};
    QLabel* m_time_label{nullptr};
    QPushButton* m_play_btn{nullptr};
    QPushButton* m_shuffle_btn{nullptr};
    QPushButton* m_repeat_btn{nullptr};
    ClickableSlider* m_progress_slider{nullptr};
    QSlider* m_volume_slider{nullptr};
    EqualizerVisualizer* m_visualizer{nullptr};

    bool m_shuffle{false};
    player::RepeatMode m_repeat{player::RepeatMode::Off};
};

}