#include "player_controls.hpp"
#include <QTime>
#include <QFile>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include "icons.hpp"

namespace ui {

void ClickableSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        if (!sr.contains(event->pos())) {
            double ratio = static_cast<double>(event->position().x()) / width();
            int val = minimum() + static_cast<int>((maximum() - minimum()) * ratio);
            setValue(val);
            emit jumpRequested(val);
            event->accept();
            return;
        }
    }
    QSlider::mousePressEvent(event);
}

PlayerControls::PlayerControls(QWidget* parent) : QWidget(parent) {
    auto* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(24, 8, 24, 8);
    main_layout->setSpacing(24);

    auto* track_layout = new QHBoxLayout();
    track_layout->setSpacing(12);

    m_cover_label = new QLabel(this);
    m_cover_label->setFixedSize(64, 64);
    m_cover_label->setStyleSheet("background-color: #1a1a1a; border-radius: 6px;");
    m_cover_label->setScaledContents(true);
    track_layout->addWidget(m_cover_label);
    track_layout->setAlignment(m_cover_label, Qt::AlignVCenter | Qt::AlignLeft);

    auto* text_layout = new QVBoxLayout();
    text_layout->setSpacing(2);
    m_title_label = new QLabel("No Track Playing", this);
    m_title_label->setStyleSheet("font-size: 14px; font-weight: bold;");
    m_title_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_artist_label = new QLabel("", this);
    m_artist_label->setStyleSheet("font-size: 12px;");
    m_artist_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    text_layout->addWidget(m_title_label);
    text_layout->addWidget(m_artist_label);
    track_layout->addLayout(text_layout, 1);
    track_layout->setAlignment(text_layout, Qt::AlignVCenter);

    main_layout->addLayout(track_layout, 1);

    auto* control_center_layout = new QVBoxLayout();
    control_center_layout->setSpacing(8);

    auto* buttons_layout = new QHBoxLayout();
    buttons_layout->setSpacing(8);
    buttons_layout->addStretch();

    m_shuffle_btn = new QPushButton(this);
    m_shuffle_btn->setFixedSize(32, 32);
    m_shuffle_btn->setFocusPolicy(Qt::NoFocus);
    m_shuffle_btn->setIcon(icons::from_svg(icons::shuffle, QColor("#8c8c8c")));
    m_shuffle_btn->setStyleSheet("background: transparent; border: none;");
    connect(m_shuffle_btn, &QPushButton::clicked, this, [this]() {
        m_shuffle = !m_shuffle;
        m_shuffle_btn->setIcon(icons::from_svg(icons::shuffle, m_shuffle ? QColor("#ffffff") : QColor("#8c8c8c")));
        emit shuffle_toggled(m_shuffle);
    });
    buttons_layout->addWidget(m_shuffle_btn);

    auto* prev_btn = new QPushButton(this);
    prev_btn->setFixedSize(32, 32);
    prev_btn->setFocusPolicy(Qt::NoFocus);
    prev_btn->setIcon(icons::from_svg(icons::skip_backward, QColor("#ffffff")));
    prev_btn->setStyleSheet("background: transparent; border: none;");
    connect(prev_btn, &QPushButton::clicked, this, &PlayerControls::prev_clicked);
    buttons_layout->addWidget(prev_btn);

    m_play_btn = new QPushButton(this);
    m_play_btn->setFixedSize(40, 40);
    m_play_btn->setFocusPolicy(Qt::NoFocus);
    m_play_btn->setIcon(icons::from_svg(icons::play, QColor("#000000")));
    m_play_btn->setStyleSheet("background-color: #ffffff; border-radius: 20px; border: none;");
    connect(m_play_btn, &QPushButton::clicked, this, [this]() {
        if (m_play_btn->toolTip() == "Play") {
            emit play_clicked();
        } else {
            emit pause_clicked();
        }
    });
    buttons_layout->addWidget(m_play_btn);

    auto* next_btn = new QPushButton(this);
    next_btn->setFixedSize(32, 32);
    next_btn->setFocusPolicy(Qt::NoFocus);
    next_btn->setIcon(icons::from_svg(icons::skip_forward, QColor("#ffffff")));
    next_btn->setStyleSheet("background: transparent; border: none;");
    connect(next_btn, &QPushButton::clicked, this, &PlayerControls::next_clicked);
    buttons_layout->addWidget(next_btn);

    m_repeat_btn = new QPushButton(this);
    m_repeat_btn->setFixedSize(32, 32);
    m_repeat_btn->setFocusPolicy(Qt::NoFocus);
    m_repeat_btn->setIcon(icons::from_svg(icons::repeat, QColor("#8c8c8c")));
    m_repeat_btn->setStyleSheet("background: transparent; border: none;");
    connect(m_repeat_btn, &QPushButton::clicked, this, [this]() {
        if (m_repeat == player::RepeatMode::Off) {
            m_repeat = player::RepeatMode::All;
            m_repeat_btn->setIcon(icons::from_svg(icons::repeat, QColor("#ffffff")));
        } else if (m_repeat == player::RepeatMode::All) {
            m_repeat = player::RepeatMode::One;
            m_repeat_btn->setIcon(icons::from_svg(icons::repeat, QColor("#ff4d4d")));
        } else {
            m_repeat = player::RepeatMode::Off;
            m_repeat_btn->setIcon(icons::from_svg(icons::repeat, QColor("#8c8c8c")));
        }
        emit repeat_changed(m_repeat);
    });
    buttons_layout->addWidget(m_repeat_btn);

    buttons_layout->addStretch();
    control_center_layout->addLayout(buttons_layout);

    auto* progress_layout = new QHBoxLayout();
    m_progress_slider = new ClickableSlider(Qt::Horizontal, this);
    m_progress_slider->setRange(0, 100);
    m_progress_slider->setFixedHeight(12);
    m_progress_slider->setFocusPolicy(Qt::NoFocus);
    
    connect(m_progress_slider, &QSlider::sliderReleased, this, [this]() {
        emit seek_requested(m_progress_slider->value());
    });
    connect(m_progress_slider, &ClickableSlider::jumpRequested, this, &PlayerControls::seek_requested);
    
    m_time_label = new QLabel("00:00 / 00:00", this);
    m_time_label->setStyleSheet("font-size: 11px;");
    
    progress_layout->addWidget(m_progress_slider, 1);
    progress_layout->addWidget(m_time_label);
    control_center_layout->addLayout(progress_layout);

    main_layout->addLayout(control_center_layout, 2);

    m_visualizer = new EqualizerVisualizer(this);
    m_visualizer->setFixedSize(120, 40);
    main_layout->addWidget(m_visualizer);

    m_volume_slider = new QSlider(Qt::Horizontal, this);
    m_volume_slider->setRange(0, 100);
    m_volume_slider->setValue(80);
    m_volume_slider->setFixedWidth(100);
    m_volume_slider->setFocusPolicy(Qt::NoFocus);
    connect(m_volume_slider, &QSlider::valueChanged, this, [this](int val) {
        emit volume_changed(static_cast<float>(val) / 100.0f);
    });
    main_layout->addWidget(m_volume_slider);
}

void PlayerControls::update_track_info(const player::Track& track) {
    m_title_label->setText(track.title);
    m_artist_label->setText(track.artist);

    QPixmap pix = icons::utils::get_cached_cover(track.file_path, 64);
    if (!pix.isNull()) {
        m_cover_label->setPixmap(pix);
    } else {
        m_cover_label->setPixmap(QPixmap());
    }
}

void PlayerControls::update_position(int seconds, int total_seconds) {
    if (m_progress_slider->isSliderDown()) return;
    
    m_progress_slider->setRange(0, total_seconds);
    m_progress_slider->setValue(seconds);

    QTime cur_t(0, 0, 0);
    QTime tot_t(0, 0, 0);
    cur_t = cur_t.addSecs(seconds);
    tot_t = tot_t.addSecs(total_seconds);

    QString format = (total_seconds >= 3600) ? "hh:mm:ss" : "mm:ss";
    m_time_label->setText(QString("%1 / %2")
        .arg(cur_t.toString(format))
        .arg(tot_t.toString(format)));
}

void PlayerControls::set_playing_state(bool playing) {
    if (playing) {
        m_play_btn->setToolTip("Pause");
        m_play_btn->setIcon(icons::from_svg(icons::pause, QColor("#000000")));
    } else {
        m_play_btn->setToolTip("Play");
        m_play_btn->setIcon(icons::from_svg(icons::play, QColor("#000000")));
    }
}

void PlayerControls::set_visualizer_stream(unsigned long bass_stream) {
    if (bass_stream) {
        m_visualizer->start(bass_stream);
    } else {
        m_visualizer->stop();
    }
}

void PlayerControls::set_volume_value(int val) {
    m_volume_slider->setValue(val);
}

}
