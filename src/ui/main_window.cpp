#include "main_window.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <QKeyEvent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace ui {
void MessageDialog::show_error(QWidget* parent, const QString& title, const QString& text) {
    MessageDialog dlg(parent, title, text);
    dlg.exec();
}

MessageDialog::MessageDialog(QWidget* parent, const QString& title, const QString& text) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 240);
    setStyleSheet("background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* title_lbl = new QLabel(title, this);
    title_lbl->setStyleSheet("font-size: 18px; font-weight: bold; color: #ff5f56; border: none; background: transparent;");
    layout->addWidget(title_lbl);

    auto* text_lbl = new QLabel(text, this);
    text_lbl->setStyleSheet("font-size: 13px; color: #ffffff; border: none; background: transparent;");
    text_lbl->setWordWrap(true);
    layout->addWidget(text_lbl, 1);

    auto* btn = new QPushButton("OK", this);
    btn->setFixedHeight(36);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff; color: #000000; border-radius: 18px; font-weight: bold; font-size: 13px; border: none;
        }
        QPushButton:hover { background-color: #e6e6e6; }
    )");
    connect(btn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(btn);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    m_pipeline.initialize();
    setup_ui();
    connect_signals();
    apply_theme("auto");

    if (m_db.discord_rpc_enabled()) {
        //https://discord.com/developers/applications
        m_discord_rpc.initialize("1523368223129735178");
    }

#if defined(Q_OS_WIN)
    RegisterHotKey(reinterpret_cast<HWND>(winId()), 1, MOD_ALT | MOD_SHIFT, VK_SPACE);
    RegisterHotKey(reinterpret_cast<HWND>(winId()), 2, MOD_ALT | MOD_SHIFT, VK_RIGHT);
    RegisterHotKey(reinterpret_cast<HWND>(winId()), 3, MOD_ALT | MOD_SHIFT, VK_LEFT);
#endif

    QTimer::singleShot(2000, this, [this]() {
        check_updates();
    });
}

MainWindow::~MainWindow() {
#if defined(Q_OS_WIN)
    UnregisterHotKey(reinterpret_cast<HWND>(winId()), 1);
    UnregisterHotKey(reinterpret_cast<HWND>(winId()), 2);
    UnregisterHotKey(reinterpret_cast<HWND>(winId()), 3);
#endif
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Space:
            if (m_pipeline.state() == player::PlaybackState::Playing) {
                m_pipeline.pause();
            } else {
                m_pipeline.resume();
            }
            event->accept();
            break;
        case Qt::Key_MediaPlay:
        case Qt::Key_MediaTogglePlayPause:
            if (m_pipeline.state() == player::PlaybackState::Playing) {
                m_pipeline.pause();
            } else {
                m_pipeline.resume();
            }
            event->accept();
            break;
        case Qt::Key_MediaPause:
            m_pipeline.pause();
            event->accept();
            break;
        case Qt::Key_MediaNext:
            m_pipeline.next();
            event->accept();
            break;
        case Qt::Key_MediaPrevious:
            m_pipeline.prev();
            event->accept();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

#if defined(Q_OS_WIN)
bool MainWindow::nativeEvent(const QByteArray&, void* message, qintptr*) {
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        int id = static_cast<int>(msg->wParam);
        if (id == 1) {
            if (m_pipeline.state() == player::PlaybackState::Playing) {
                m_pipeline.pause();
            } else {
                m_pipeline.resume();
            }
            return true;
        } else if (id == 2) {
            m_pipeline.next();
            return true;
        } else if (id == 3) {
            m_pipeline.prev();
            return true;
        }
    }
    return false;
}
#endif

void MainWindow::setup_ui() {
    setMinimumSize(1200, 800);
    setWindowTitle("");//pustota
    auto* central_widget = new QWidget(this);
    central_widget->setObjectName("central_widget");
    setCentralWidget(central_widget);
    auto* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    m_title_bar = new TitleBar(this);
    m_title_bar->setObjectName("title_bar");
    main_layout->addWidget(m_title_bar);
    auto* content_layout = new QHBoxLayout();
    content_layout->setSpacing(0);
    m_sidebar = new Sidebar(central_widget);
    m_sidebar->setFixedWidth(240);
    m_sidebar->setObjectName("sidebar");
    content_layout->addWidget(m_sidebar);
    m_stacked_widget = new QStackedWidget(central_widget);
    m_stacked_widget->setObjectName("stacked_widget");
    content_layout->addWidget(m_stacked_widget);
    m_library_view = new LibraryView(&m_db, m_stacked_widget);
    m_playlists_view = new PlaylistsView(&m_db, m_stacked_widget);
    m_discover_view = new DiscoverView(&m_dl, m_stacked_widget);
    m_downloads_view = new DownloadsView(&m_dl, m_stacked_widget);
    m_settings_view = new SettingsView(m_stacked_widget);
    m_playlist_view = new PlaylistView(&m_db, m_stacked_widget);
    m_settings_view->initialize_format_selection(m_db.save_format(), [this](const QString& format) {
        m_db.set_save_format(format);
    });

    m_settings_view->initialize_theme_selection("Auto", [this](const QString& theme) {
        apply_theme(theme);
    });

    m_settings_view->update_config_dir_label(m_db.storage_dir());

    m_settings_view->initialize_settings(
        m_db.crossfade(),
        m_db.gapless_playback(),
        m_db.normalize_volume(),
        m_db.auto_download_favorites(),
        m_db.wifi_only(),
        m_db.equalizer_enabled(),
        m_db.discord_rpc_enabled(),
        m_db.save_format().toUpper(),
        "Auto",
        m_db.storage_dir()
    );

    m_stacked_widget->addWidget(m_library_view);
    m_stacked_widget->addWidget(m_playlists_view);
    m_stacked_widget->addWidget(m_discover_view);
    m_stacked_widget->addWidget(m_downloads_view);
    m_stacked_widget->addWidget(m_settings_view);
    m_stacked_widget->addWidget(m_playlist_view);

    main_layout->addLayout(content_layout, 1);

    m_controls = new PlayerControls(central_widget);
    m_controls->setFixedHeight(90);
    m_controls->setObjectName("player_controls");
    main_layout->addWidget(m_controls);

    m_controls->set_volume_value(static_cast<int>(m_db.volume() * 100.0f));
    m_pipeline.set_volume(m_db.volume());
    m_pipeline.set_normalize(m_db.normalize_volume());
}

void MainWindow::connect_signals() {
    connect(m_sidebar, &Sidebar::navigation_changed, this, &MainWindow::switch_page);

    connect(m_library_view, &LibraryView::play_requested, this, [this](const std::vector<player::Track>& queue, size_t index) {
        m_pipeline.play_queue(queue, index);
        m_active_play_source = "library";
    });

    connect(m_library_view, &LibraryView::queue_updated, this, [this](const std::vector<player::Track>& queue) {
        if (m_active_play_source == "library") {
            m_pipeline.update_queue(queue);
        }
    });

    connect(m_playlist_view, &PlaylistView::play_requested, this, [this](const std::vector<player::Track>& queue, size_t index) {
        m_pipeline.play_queue(queue, index);
        m_active_play_source = "playlist";
    });

    connect(m_playlist_view, &PlaylistView::queue_updated, this, [this](const std::vector<player::Track>& queue) {
        if (m_active_play_source == "playlist") {
            m_pipeline.update_queue(queue);
        }
    });

    connect(&m_pipeline, &player::AudioPipeline::track_changed, this, [this](const player::Track& track) {
        m_controls->update_track_info(track);
        m_controls->set_playing_state(true);
        if (m_db.equalizer_enabled()) {
            m_controls->set_visualizer_stream(m_pipeline.bass_stream());
        } else {
            m_controls->set_visualizer_stream(0);
        }
        
        if (m_db.discord_rpc_enabled()) {
            m_discord_rpc.update_presence(track, 0);
        }
    }, Qt::QueuedConnection);

    connect(&m_pipeline, &player::AudioPipeline::position_updated, this, [this](std::chrono::seconds pos) {
        if (m_pipeline.state() != player::PlaybackState::Playing) return;

        int total = static_cast<int>(m_pipeline.current_track().duration.count());
        m_controls->update_position(static_cast<int>(pos.count()), total);
    }, Qt::QueuedConnection);

    connect(&m_pipeline, &player::AudioPipeline::state_changed, this, [this](player::PlaybackState state) {
        m_controls->set_playing_state(state == player::PlaybackState::Playing);
        if (state == player::PlaybackState::Stopped) {
            m_controls->set_visualizer_stream(0);
            if (m_db.discord_rpc_enabled()) {
                m_discord_rpc.clear_presence();
            }
        } else if (state == player::PlaybackState::Paused) {
            if (m_db.discord_rpc_enabled()) {
                m_discord_rpc.clear_presence();
            }
        }
    }, Qt::QueuedConnection);

    connect(m_controls, &PlayerControls::play_clicked, this, [this]() {
        m_pipeline.resume();
        if (m_db.discord_rpc_enabled()) {
            m_discord_rpc.update_presence(m_pipeline.current_track(), m_pipeline.current_position().count());
        }
    });

    connect(m_controls, &PlayerControls::pause_clicked, this, [this]() {
        m_pipeline.pause();
        if (m_db.discord_rpc_enabled()) {
            m_discord_rpc.clear_presence();
        }
    });

    connect(m_controls, &PlayerControls::seek_requested, this, [this](int seconds) {
        m_pipeline.seek(seconds);
        if (m_db.discord_rpc_enabled()) {
            m_discord_rpc.update_presence(m_pipeline.current_track(), seconds);
        }
    });
    
    connect(m_controls, &PlayerControls::volume_changed, this, [this](float vol) {
        m_pipeline.set_volume(vol);
        m_db.set_volume(vol);
    });
    
    connect(m_controls, &PlayerControls::prev_clicked, &m_pipeline, &player::AudioPipeline::prev);
    connect(m_controls, &PlayerControls::next_clicked, &m_pipeline, &player::AudioPipeline::next);

    connect(m_controls, &PlayerControls::shuffle_toggled, &m_pipeline, &player::AudioPipeline::set_shuffle);
    connect(m_controls, &PlayerControls::repeat_changed, &m_pipeline, &player::AudioPipeline::set_repeat);

    connect(m_settings_view, &SettingsView::crossfade_toggled, this, [this](bool checked) {
        m_db.set_crossfade(checked);
    });
    connect(m_settings_view, &SettingsView::gapless_toggled, this, [this](bool checked) {
        m_db.set_gapless_playback(checked);
    });
    connect(m_settings_view, &SettingsView::normalize_toggled, this, [this](bool checked) {
        m_db.set_normalize_volume(checked);
        m_pipeline.set_normalize(checked);
    });
    connect(m_settings_view, &SettingsView::auto_download_toggled, this, [this](bool checked) {
        m_db.set_auto_download_favorites(checked);
    });
    connect(m_settings_view, &SettingsView::wifi_only_toggled, this, [this](bool checked) {
        m_db.set_wifi_only(checked);
    });
    connect(m_settings_view, &SettingsView::equalizer_toggled, this, [this](bool checked) {
        m_db.set_equalizer_enabled(checked);
        m_controls->set_visualizer_stream(checked ? m_pipeline.bass_stream() : 0);
    });
    connect(m_settings_view, &SettingsView::discord_rpc_toggled, this, [this](bool checked) {
        m_db.set_discord_rpc_enabled(checked);
        if (checked) {
            m_discord_rpc.initialize("1523368223129735178");
            if (m_pipeline.state() == player::PlaybackState::Playing) {
                m_discord_rpc.update_presence(m_pipeline.current_track(), m_pipeline.current_position().count());
            }
        } else {
            m_discord_rpc.shutdown();
        }
    });

    connect(m_settings_view, &SettingsView::config_dir_requested, this, [this]() {
        QString current_dir = m_db.storage_dir();
        QString new_dir = QFileDialog::getExistingDirectory(this, "Select Config Folder", current_dir);
        if (!new_dir.isEmpty() && new_dir != current_dir) {
            QMessageBox msg_box(this);
            msg_box.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            msg_box.setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a; border-radius: 12px; font-size: 13px;");
            msg_box.setWindowTitle("Transfer Library Data?");
            msg_box.setText("Do you want to copy your current library configuration to the new folder, or start new file?");
            
            QPushButton* transfer_btn = msg_box.addButton("Transfer Existing", QMessageBox::YesRole);
            QPushButton* fresh_btn = msg_box.addButton("Start New", QMessageBox::NoRole);
            QPushButton* cancel_btn = msg_box.addButton(QMessageBox::Cancel);

            transfer_btn->setFocusPolicy(Qt::NoFocus);
            fresh_btn->setFocusPolicy(Qt::NoFocus);
            cancel_btn->setFocusPolicy(Qt::NoFocus);

            transfer_btn->setStyleSheet("background-color: #ffffff; color: #000000; border-radius: 6px; padding: 6px 14px; font-weight: bold; border: none;");
            fresh_btn->setStyleSheet("background-color: #1a1a1a; color: #ffffff; border-radius: 6px; padding: 6px 14px; font-weight: bold; border: 1px solid #333333;");
            cancel_btn->setStyleSheet("background-color: #1a1a1a; color: #ffffff; border-radius: 6px; padding: 6px 14px; font-weight: bold; border: 1px solid #333333;");

            msg_box.exec();

            if (msg_box.clickedButton() == cancel_btn) {
                return;
            }

            bool copy_existing = (msg_box.clickedButton() == transfer_btn);
            m_db.change_storage_dir(new_dir, copy_existing);
            m_settings_view->update_config_dir_label(new_dir);
            m_library_view->refresh();
        }
    });

    connect(m_discover_view, &DiscoverView::download_requested, this, [this](const player::WebTrack& track) {
        auto* self = this;
        m_dl.download(track, "Downloads", m_db.save_format(), [self, track](bool ok, QString err_msg, player::Track downloaded) {
            if (ok) {
                self->m_db.add_track(downloaded);
            } else {
                MessageDialog::show_error(self, "Download Error", 
                    QString("Failed to download \"%1\".\n\nReason:\n%2")
                    .arg(track.title)
                    .arg(err_msg.isEmpty() ? "Connection timeout or geo-restriction" : err_msg)
                );
            }
        });
        m_sidebar->navigation_changed(3);
    });

    connect(m_discover_view, &DiscoverView::batch_download_requested, this, [this](const std::vector<player::WebTrack>& tracks) {
        auto* self = this;
        for (const auto& track : tracks) {
            m_dl.download(track, "Downloads", m_db.save_format(), [self](bool ok, QString, player::Track downloaded) {
                if (ok) {
                    self->m_db.add_track(downloaded);
                }
            });
        }
        m_sidebar->navigation_changed(3);
    });

    auto* space_shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    space_shortcut->setContext(Qt::WindowShortcut);
    connect(space_shortcut, &QShortcut::activated, this, [this]() {
        if (m_pipeline.state() == player::PlaybackState::Playing) {
            m_pipeline.pause();
        } else if (m_pipeline.state() == player::PlaybackState::Paused) {
            m_pipeline.resume();
        }
    });

    auto* ctrl_f_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    ctrl_f_shortcut->setContext(Qt::WindowShortcut);
    connect(ctrl_f_shortcut, &QShortcut::activated, this, [this]() {
        int idx = m_stacked_widget->currentIndex();
        if (idx == 0 && m_library_view) {
            m_library_view->toggle_search();
        } else if (idx == 5 && m_playlist_view) {
            m_playlist_view->toggle_search();
        }
    });
}

void MainWindow::switch_page(int index) {
    QWidget* widget = m_stacked_widget->widget(index);
    if (!widget) return;

    if (widget->graphicsEffect()) {
        widget->setGraphicsEffect(nullptr);
    }
    
    auto* effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);
    
    m_stacked_widget->setCurrentIndex(index);
    
    auto* anim = new QPropertyAnimation(effect, "opacity", widget);
    anim->setDuration(220);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    
    connect(anim, &QPropertyAnimation::finished, this, [widget, effect]() {
        QTimer::singleShot(0, widget, [widget, effect]() {
            if (widget->graphicsEffect() == effect) {
                widget->setGraphicsEffect(nullptr);
            }
        });
    }, Qt::QueuedConnection);
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::open_playlist_detail(const QString& name) {
    m_playlist_view->open_playlist(name);
    switch_page(5);
}

void MainWindow::apply_theme(const QString& theme) {
    auto switches = findChildren<ToggleSwitch*>();
    QColor bg_color = QColor("#0c0c0c");

    if (theme == "paper") {
        bg_color = QColor("#faf8f5");
        for (auto* sw : switches) {
            sw->set_colors(QColor("#1a1a1a"), QColor("#e0d7c5"), QColor("#fcfbfa"));
        }
        setStyleSheet(R"(
            QWidget { font-family: "Segoe UI", "Arial"; color: #1a1a1a; }
            QMainWindow, QWidget#central_widget { background-color: #faf8f5; }
            QStackedWidget#stacked_widget { background-color: #faf8f5; }
            QWidget#title_bar { background-color: #f1ede4; border-bottom: 1px solid #e0d7c5; }
            QWidget#sidebar { background-color: #f1ede4; border-right: 1px solid #e0d7c5; }
            QWidget#player_controls { background-color: #f1ede4; border-top: 1px solid #e0d7c5; }
            QWidget#playlist_header { background-color: rgba(241, 237, 228, 120); border-bottom: 1px solid #e0d7c5; }
            QTableView { background-color: #faf8f5; gridline-color: transparent; border: none; color: #1a1a1a; }
            QTableView::item { border-bottom: 1px solid #e0d7c5; padding: 12px; }
            QTableView::item:selected { background-color: #e8e3d5; }
            QHeaderView::section { background-color: #faf8f5; color: #706a5e; border: none; font-weight: bold; font-size: 11px; padding: 0px 8px; height: 32px; }
            QHeaderView::section:hover { color: #1a1a1a; }
            QHeaderView::down-arrow, QHeaderView::up-arrow { width: 0px; height: 0px; border: none; background: transparent; }
            QFrame#settings_card { background-color: #fcfbfa; border: 1px solid #e0d7c5; border-radius: 12px; }
            QFrame#playlist_card { background-color: #fcfbfa; border: 1px solid #e0d7c5; border-radius: 12px; }
            QFrame#playlist_card:hover { background-color: #e8e3d5; }
            QPushButton { background-color: #fcfbfa; border: 1px solid #e0d7c5; border-radius: 6px; padding: 0px 12px; color: #1a1a1a; font-weight: bold; }
            QPushButton:hover { background-color: #e8e3d5; }
            QPushButton:disabled { background-color: #eae5dc; border-color: #dcd5c8; color: #b0aaa0; }
            QLineEdit { background-color: #fcfbfa; border: 1px solid #e0d7c5; border-radius: 6px; padding: 0px 12px; color: #1a1a1a; }
            QSlider::groove:horizontal { height: 4px; background: #e0d7c5; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #1a1a1a; border-radius: 2px; }
            QSlider::handle:horizontal { background: #1a1a1a; width: 10px; height: 10px; margin-top: -3px; margin-bottom: -3px; border-radius: 5px; }
            QSlider::handle:horizontal:hover { width: 12px; height: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; }
            QScrollBar:vertical { width: 0px; background: transparent; }
            QScrollBar:horizontal { height: 0px; background: transparent; }
            QLabel { color: #1a1a1a; border: none; background: transparent; }
            QLabel:disabled { color: #b0aaa0; }
            QMessageBox { background-color: #f1ede4; border: 1px solid #e0d7c5; }
            QMessageBox QLabel { color: #1a1a1a; }
            QMessageBox QPushButton { background-color: #fcfbfa; color: #1a1a1a; border: 1px solid #e0d7c5; border-radius: 6px; padding: 6px 12px; }
            QMessageBox QPushButton:hover { background-color: #e8e3d5; }
        )");
    } else if (theme == "sepia") {
        for (auto* sw : switches) {
            sw->set_colors(QColor("#433422"), QColor("#d8c8b0"), QColor("#FAF5E8"));
        }
        setStyleSheet(R"(
            QWidget { font-family: "Segoe UI", "Arial"; color: #433422; }
            QMainWindow, QWidget#central_widget { background-color: #f4ecd8; }
            QStackedWidget#stacked_widget { background-color: #f4ecd8; }
            QWidget#title_bar { background-color: #eadeca; border-bottom: 1px solid #d8c8b0; }
            QWidget#sidebar { background-color: #eadeca; border-right: 1px solid #d8c8b0; }
            QWidget#player_controls { background-color: #eadeca; border-top: 1px solid #d8c8b0; }
            QWidget#playlist_header { background-color: rgba(234, 222, 202, 120); border-bottom: 1px solid #d8c8b0; }
            QTableView { background-color: #f4ecd8; gridline-color: transparent; border: none; color: #433422; }
            QTableView::item { border-bottom: 1px solid #d8c8b0; padding: 12px; }
            QTableView::item:selected { background-color: #dbcca9; }
            QHeaderView::section { background-color: #f4ecd8; color: #7e6850; border: none; font-weight: bold; font-size: 11px; padding: 0px 8px; height: 32px; }
            QHeaderView::section:hover { color: #433422; }
            QHeaderView::down-arrow, QHeaderView::up-arrow { width: 0px; height: 0px; border: none; background: transparent; }
            QFrame#settings_card { background-color: #FAF5E8; border: 1px solid #d8c8b0; border-radius: 12px; }
            QFrame#playlist_card { background-color: #FAF5E8; border: 1px solid #d8c8b0; border-radius: 12px; }
            QFrame#playlist_card:hover { background-color: #dbcca9; }
            QPushButton { background-color: #FAF5E8; border: 1px solid #d8c8b0; border-radius: 6px; padding: 0px 12px; color: #433422; font-weight: bold; }
            QPushButton:hover { background-color: #dbcca9; }
            QPushButton:disabled { background-color: #e0d5c0; border-color: #ccbc9a; color: #a69580; }
            QLineEdit { background-color: #FAF5E8; border: 1px solid #d8c8b0; border-radius: 6px; padding: 0px 12px; color: #433422; }
            QSlider::groove:horizontal { height: 4px; background: #d8c8b0; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #433422; border-radius: 2px; }
            QSlider::handle:horizontal { background: #433422; width: 10px; height: 10px; margin-top: -3px; margin-bottom: -3px; border-radius: 5px; }
            QSlider::handle:horizontal:hover { width: 12px; height: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; }
            QScrollBar:vertical { width: 0px; background: transparent; }
            QScrollBar:horizontal { height: 0px; background: transparent; }
            QLabel { color: #433422; border: none; background: transparent; }
            QLabel:disabled { color: #a69580; }
            QMessageBox { background-color: #eadeca; border: 1px solid #d8c8b0; }
            QMessageBox QLabel { color: #433422; }
            QMessageBox QPushButton { background-color: #FAF5E8; color: #433422; border: 1px solid #d8c8b0; border-radius: 6px; padding: 6px 12px; }
            QMessageBox QPushButton:hover { background-color: #dbcca9; }
        )");
    } else {
        for (auto* sw : switches) {
            sw->set_colors(QColor("#ffffff"), QColor("#262626"), QColor("#000000"));
        }
        setStyleSheet(R"(
            QWidget { font-family: "Segoe UI", "Arial"; color: #ffffff; }
            QMainWindow, QWidget#central_widget { background-color: #0c0c0c; }
            QStackedWidget#stacked_widget { background-color: #0c0c0c; }
            QWidget#title_bar { background-color: #0c0c0c; border-bottom: 1px solid #141414; }
            QWidget#sidebar { background-color: #080808; border-right: 1px solid #141414; }
            QWidget#player_controls { background-color: #080808; border-top: 1px solid #141414; }
            QWidget#playlist_header { background-color: rgba(12, 12, 12, 120); border-bottom: 1px solid #141414; }
            QTableView { background-color: #0c0c0c; gridline-color: transparent; border: none; color: #ffffff; }
            QTableView::item { border-bottom: 1px solid #141414; padding: 12px; }
            QTableView::item:selected { background-color: #222222; }
            QHeaderView::section { background-color: #0c0c0c; color: #8c8c8c; border: none; font-weight: bold; font-size: 11px; padding: 0px 8px; height: 32px; }
            QHeaderView::section:hover { color: #ffffff; }
            QHeaderView::down-arrow, QHeaderView::up-arrow { width: 0px; height: 0px; border: none; background: transparent; }
            QFrame#settings_card { background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px; }
            QFrame#playlist_card { background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px; }
            QFrame#playlist_card:hover { background-color: #1a1a1a; border-color: #262626; }
            QPushButton { background-color: #1a1a1a; border: 2px solid #262626; border-radius: 6px; padding: 0px 12px; color: #ffffff; font-weight: bold; }
            QPushButton:hover { background-color: #262626; }
            QPushButton:disabled { background-color: #121212; border-color: #1a1a1a; color: #444444; }
            QLineEdit { background-color: #1a1a1a; border: 2px solid #262626; border-radius: 6px; padding: 0px 12px; color: #ffffff; }
            QSlider::groove:horizontal { height: 4px; background: #2a2a2a; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #ffffff; border-radius: 2px; }
            QSlider::handle:horizontal { background: #ffffff; width: 10px; height: 10px; margin-top: -3px; margin-bottom: -3px; border-radius: 5px; }
            QSlider::handle:horizontal:hover { width: 12px; height: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; }
            QScrollBar:vertical { width: 0px; background: transparent; }
            QScrollBar:horizontal { height: 0px; background: transparent; }
            QLabel { color: #ffffff; border: none; background: transparent; }
            QLabel:disabled { color: #444444; }
            QMessageBox { background-color: #121212; border: 1px solid #1a1a1a; }
            QMessageBox QLabel { color: #ffffff; }
            QMessageBox QPushButton { background-color: #1a1a1a; color: #ffffff; border: 1px solid #333333; border-radius: 6px; padding: 6px 12px; }
            QMessageBox QPushButton:hover { background-color: #262626; }
        )");
    }

    m_playlist_view->set_theme_colors(bg_color);
}

void MainWindow::check_updates() {
    auto* nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("https://api.github.com/repos/linwys/Maya/releases/latest"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "Maya-Player");
    
    connect(nam, &QNetworkAccessManager::finished, this, [this, nam](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString latest_tag = obj["tag_name"].toString().trimmed();
                
                auto sanitize = [](QString tag) {
                    if (tag.startsWith('v', Qt::CaseInsensitive)) {
                        tag.remove(0, 1);
                    }
                    return tag.trimmed().toLower();
                };
                
                if (sanitize(latest_tag) != sanitize(TitleBar::VERSION)) {
                    MessageDialog::show_error(this, "Update Available", 
                        QString("A new version of Maya is available!\n\nCurrent: %1\nLatest: %2\n\nDownload the new release on GitHub.")
                        .arg(TitleBar::VERSION)
                        .arg(latest_tag)
                    );
                }
            }
        }
        reply->deleteLater();
        nam->deleteLater();
    });
    nam->get(req);
}

}
