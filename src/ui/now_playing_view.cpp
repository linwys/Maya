#include "now_playing_view.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QTime>
#include <QFileDialog>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include "icons.hpp"
#include "bytes.hpp"

namespace ui {

NowPlayingView::NowPlayingView(player::AudioPipeline* pipeline, player::Db* db, QWidget* parent)
    : QWidget(parent), m_pipeline(pipeline), m_db(db) {
    setup_ui();
    connect_signals();
}

void NowPlayingView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Slash) {
        emit back_requested();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void NowPlayingView::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a;");
    
    QAction* edit_bg = menu.addAction("Edit background");
    
    QString custom_path = QDir(m_db->storage_dir()).filePath("bgfile.png");
    bool has_custom_bg = QFile::exists(custom_path);
    QAction* restore_bg = nullptr;
    
//show only if custom bg active
    if (has_custom_bg) {
        restore_bg = menu.addAction("Restore default");
    }
    
    QAction* selected = menu.exec(event->globalPos());
    if (selected == edit_bg) {
        QString file = QFileDialog::getOpenFileName(this, "Select background image", "", "Images (*.png *.jpg *.jpeg *.webp)");
        if (!file.isEmpty()) {
            QString target_dir = m_db->storage_dir();
            QDir().mkpath(target_dir);
            QString dest_path = QDir(target_dir).filePath("bgfile.png");
            
            if (QFile::exists(dest_path)) {
                QFile::remove(dest_path);
            }
            if (QFile::copy(file, dest_path)) {
                load_background();
                update();
            }
        }
    } else if (restore_bg && selected == restore_bg) {
        if (QFile::exists(custom_path)) {
            QFile::remove(custom_path);
        }
        load_background(); // +
        update();
    }
}

void NowPlayingView::setup_ui() {
    setFocusPolicy(Qt::StrongFocus);

    m_back_btn = new QPushButton(this);
    m_back_btn->setGeometry(32, 32, 40, 40);
    m_back_btn->setFocusPolicy(Qt::NoFocus);
    m_back_btn->setCursor(Qt::PointingHandCursor);
    m_back_btn->setIcon(icons::from_svg(icons::skip_backward, QColor("#ffffff")));
    m_back_btn->setStyleSheet("background-color: rgba(255, 255, 255, 12); border: 1px solid rgba(255, 255, 255, 20); border-radius: 20px;");
    connect(m_back_btn, &QPushButton::clicked, this, &NowPlayingView::back_requested);

    auto* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(80, 80, 80, 80);
    main_layout->setSpacing(64);

    m_cover_label = new QLabel(this);
    m_cover_label->setFixedSize(400, 400);
    m_cover_label->setStyleSheet("background-color: #1a1a1a; border-radius: 16px;");
    m_cover_label->setScaledContents(true);
    main_layout->addWidget(m_cover_label, 0, Qt::AlignVCenter);

    auto* right_layout = new QVBoxLayout();
    right_layout->setSpacing(24);
    right_layout->setAlignment(Qt::AlignVCenter);

    auto* meta_layout = new QVBoxLayout();
    meta_layout->setSpacing(4);
    m_title_label = new MarqueeLabel("No Track Playing", this);
    m_title_label->setStyleSheet("font-size: 36px; font-weight: bold; color: #ffffff; background: transparent; border: none;");
    m_artist_label = new MarqueeLabel("", this);
    m_artist_label->setStyleSheet("font-size: 18px; color: #b3b3b3; background: transparent; border: none;");
    meta_layout->addWidget(m_title_label);
    meta_layout->addWidget(m_artist_label);
    right_layout->addLayout(meta_layout);

    auto* progress_section = new QVBoxLayout();
    progress_section->setSpacing(8);
    m_progress_slider = new ClickableSlider(Qt::Horizontal, this);
    m_progress_slider->setRange(0, 100);
    m_progress_slider->setFixedHeight(12);
    m_progress_slider->setFocusPolicy(Qt::NoFocus);
    m_progress_slider->setStyleSheet(R"(
        QSlider::groove:horizontal { height: 4px; background: rgba(255, 255, 255, 40); border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #ffffff; border-radius: 2px; }
        QSlider::handle:horizontal { background: #ffffff; width: 12px; height: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; }
    )");

    connect(m_progress_slider, &QSlider::sliderReleased, this, [this]() {
        m_pipeline->seek(m_progress_slider->value());
    });
    connect(m_progress_slider, &ClickableSlider::jumpRequested, this, [this](int secs) {
        m_pipeline->seek(secs);
    });

    m_time_label = new QLabel("00:00 / 00:00", this);
    m_time_label->setStyleSheet("font-size: 12px; color: #b3b3b3; background: transparent; border: none;");
    progress_section->addWidget(m_progress_slider);
    progress_section->addWidget(m_time_label, 0, Qt::AlignRight);
    right_layout->addLayout(progress_section);

    auto* ctrl_layout = new QHBoxLayout();
    ctrl_layout->setSpacing(16);
    ctrl_layout->setAlignment(Qt::AlignCenter);

    m_shuffle_btn = new QPushButton(this);
    m_shuffle_btn->setFixedSize(40, 40);
    m_shuffle_btn->setFocusPolicy(Qt::NoFocus);
    m_shuffle_btn->setCursor(Qt::PointingHandCursor);
    m_shuffle_btn->setIcon(icons::from_svg(icons::shuffle, QColor("#8c8c8c")));
    m_shuffle_btn->setStyleSheet("background: transparent; border: none;");
    connect(m_shuffle_btn, &QPushButton::clicked, this, [this]() {
        m_shuffle = !m_shuffle;
        m_shuffle_btn->setIcon(icons::from_svg(icons::shuffle, m_shuffle ? QColor("#ffffff") : QColor("#8c8c8c")));
        m_pipeline->set_shuffle(m_shuffle);
    });
    ctrl_layout->addWidget(m_shuffle_btn);

    m_prev_btn = new QPushButton(this);
    m_prev_btn->setFixedSize(40, 40);
    m_prev_btn->setFocusPolicy(Qt::NoFocus);
    m_prev_btn->setCursor(Qt::PointingHandCursor);
    m_prev_btn->setIcon(icons::from_svg(icons::skip_backward, QColor("#ffffff")));
    m_prev_btn->setStyleSheet("background: transparent; border: none;");
    connect(m_prev_btn, &QPushButton::clicked, m_pipeline, &player::AudioPipeline::prev);
    ctrl_layout->addWidget(m_prev_btn);

    m_play_btn = new QPushButton(this);
    m_play_btn->setFixedSize(56, 56);
    m_play_btn->setFocusPolicy(Qt::NoFocus);
    m_play_btn->setCursor(Qt::PointingHandCursor);
    m_play_btn->setIcon(icons::from_svg(icons::play, QColor("#000000")));
    m_play_btn->setStyleSheet("background-color: #ffffff; border-radius: 28px; border: none;");
    connect(m_play_btn, &QPushButton::clicked, this, [this]() {
        if (m_pipeline->state() == player::PlaybackState::Playing) {
            m_pipeline->pause();
        } else {
            m_pipeline->resume();
        }
    });
    ctrl_layout->addWidget(m_play_btn);

    m_next_btn = new QPushButton(this);
    m_next_btn->setFixedSize(40, 40);
    m_next_btn->setFocusPolicy(Qt::NoFocus);
    m_next_btn->setCursor(Qt::PointingHandCursor);
    m_next_btn->setIcon(icons::from_svg(icons::skip_forward, QColor("#ffffff")));
    m_next_btn->setStyleSheet("background: transparent; border: none;");
    connect(m_next_btn, &QPushButton::clicked, this, [this]() {
        m_pipeline->next(true);
    });
    ctrl_layout->addWidget(m_next_btn);

    m_repeat_btn = new QPushButton(this);
    m_repeat_btn->setFixedSize(40, 40);
    m_repeat_btn->setFocusPolicy(Qt::NoFocus);
    m_repeat_btn->setCursor(Qt::PointingHandCursor);
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
        m_pipeline->set_repeat(m_repeat);
    });
    ctrl_layout->addWidget(m_repeat_btn);

    right_layout->addLayout(ctrl_layout);

    auto* volume_section = new QHBoxLayout();
    volume_section->setSpacing(8);
    volume_section->addStretch();

    auto* vol_icon = new QLabel(this);
    vol_icon->setPixmap(icons::from_svg(icons::volume, QColor("#b3b3b3")).pixmap(16, 16));
    volume_section->addWidget(vol_icon);

    m_volume_slider = new QSlider(Qt::Horizontal, this);
    m_volume_slider->setRange(0, 100);
    m_volume_slider->setFixedWidth(120);
    m_volume_slider->setFocusPolicy(Qt::NoFocus);
    m_volume_slider->setStyleSheet(R"(
        QSlider::groove:horizontal { height: 4px; background: rgba(255, 255, 255, 40); border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #ffffff; border-radius: 2px; }
        QSlider::handle:horizontal { background: #ffffff; width: 10px; height: 10px; margin-top: -3px; margin-bottom: -3px; border-radius: 5px; }
    )");
    connect(m_volume_slider, &QSlider::valueChanged, this, [this](int val) {
        float vol = static_cast<float>(val) / 100.0f;
        m_pipeline->set_volume(vol);
        m_db->set_volume(vol);
    });
    volume_section->addWidget(m_volume_slider);
    right_layout->addLayout(volume_section);
    //volume_section->addStretch();
    main_layout->addLayout(right_layout, 1);
}

void NowPlayingView::connect_signals() {
    m_volume_slider->setValue(static_cast<int>(m_db->volume() * 100.0f));
}

void NowPlayingView::update_track_info(const player::Track& track) {
    m_title_label->setText(track.title);
    m_artist_label->setText(track.artist);

    QPixmap pix = icons::utils::get_cached_cover(track.file_path, 400);
    if (!pix.isNull()) {
        m_cover_label->setPixmap(pix);
    } else {
        m_cover_label->setPixmap(QPixmap());
    }
}

void NowPlayingView::update_position(int seconds, int total_seconds) {
    if (m_progress_slider->isSliderDown()) return;
    m_progress_slider->setRange(0, total_seconds);
    m_progress_slider->setValue(seconds);

    QTime cur_t(0, 0, 0);
    QTime tot_t(0, 0, 0);
    cur_t = cur_t.addSecs(seconds);
    tot_t = tot_t.addSecs(total_seconds);

    QString format = (total_seconds >= 3600) ? "hh:mm:ss" : "mm:ss";
    m_time_label->setText(QString("%1 / %2").arg(cur_t.toString(format)).arg(tot_t.toString(format)));
}

void NowPlayingView::set_playing_state(bool playing) {
    if (playing) {
        m_play_btn->setIcon(icons::from_svg(icons::pause, QColor("#000000")));
    } else {
        m_play_btn->setIcon(icons::from_svg(icons::play, QColor("#000000")));
    }
}

void NowPlayingView::load_background() {
    QString custom_path = QDir(m_db->storage_dir()).filePath("bgfile.png");
    if (QFile::exists(custom_path)) {
        m_custom_bg_image.load(custom_path);
    } else {
        m_custom_bg_image.loadFromData(background_data, sizeof(background_data));
    }

    if (!m_custom_bg_image.isNull()) {
        m_scaled_bg_image = m_custom_bg_image.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
}

void NowPlayingView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    load_background();
    m_volume_slider->setValue(static_cast<int>(m_db->volume() * 100.0f));
}

void NowPlayingView::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    m_scaled_bg_image = QPixmap();
    m_custom_bg_image = QPixmap();
}

void NowPlayingView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (!m_custom_bg_image.isNull()) {
        m_scaled_bg_image = m_custom_bg_image.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        update();
    }
}

void NowPlayingView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_scaled_bg_image.isNull()) {
        int x = (width() - m_scaled_bg_image.width()) / 2;
        int y = (height() - m_scaled_bg_image.height()) / 2;
        painter.drawPixmap(x, y, m_scaled_bg_image);
    } else {
        painter.fillRect(rect(), QColor("#0c0c0c"));
    }

    painter.fillRect(rect(), QColor(12, 12, 12, 210));
}

}