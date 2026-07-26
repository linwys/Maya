#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "marquee_label.hpp"
#include "player_controls.hpp"
#include "player/audio_pipeline.hpp"
#include "player/db.hpp"

namespace ui {

class NowPlayingView : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingView(player::AudioPipeline* pipeline, player::Db* db, QWidget* parent = nullptr);
    ~NowPlayingView() override = default;

    void update_track_info(const player::Track& track);
    void update_position(int seconds, int total_seconds);
    void set_playing_state(bool playing);

signals:
    void back_requested();

protected:
    void paintEvent(QPaintEvent* event) override;   // +
    void showEvent(QShowEvent* event) override; // +
    void hideEvent(QHideEvent* event) override; // +
    void resizeEvent(QResizeEvent* event) override; // +
    void keyPressEvent(QKeyEvent* event) override; // +
    void contextMenuEvent(QContextMenuEvent* event) override;   // +

private:
    void setup_ui(); // +
    void connect_signals(); // +
    void load_background();         // +

    player::AudioPipeline* m_pipeline{nullptr};
    player::Db* m_db{nullptr};

    QLabel* m_cover_label{nullptr};
    MarqueeLabel* m_title_label{nullptr};
    MarqueeLabel* m_artist_label{nullptr};
    QLabel* m_time_label{nullptr};
    
    QPushButton* m_play_btn{nullptr};
    QPushButton* m_shuffle_btn{nullptr};
    QPushButton* m_repeat_btn{nullptr};
    QPushButton* m_prev_btn{nullptr};
    QPushButton* m_next_btn{nullptr};
    ClickableSlider* m_progress_slider{nullptr};
    QSlider* m_volume_slider{nullptr};
    QPushButton* m_back_btn{nullptr};

    QPixmap m_custom_bg_image;
    QPixmap m_scaled_bg_image;
    bool m_shuffle{false};
    player::RepeatMode m_repeat{player::RepeatMode::Off};
};

}