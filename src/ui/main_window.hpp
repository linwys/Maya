#pragma once
#include <QMainWindow>
#include <QDialog>
#include <QStackedWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QShortcut>
#include "title_bar.hpp"
#include "sidebar.hpp"
#include "player_controls.hpp"
#include "library_view.hpp"
#include "playlists_view.hpp"
#include "discover_view.hpp"
#include "downloads_view.hpp"
#include "settings_view.hpp"
#include "playlist_view.hpp"
#include "player/audio_pipeline.hpp"
#include "player/db.hpp"
#include "player/dl_mgr.hpp"
#include "player/discord_rpc.hpp"

namespace ui {

class MessageDialog : public QDialog {
    Q_OBJECT
public:
    static void show_error(QWidget* parent, const QString& title, const QString& text);
    MessageDialog(QWidget* parent, const QString& title, const QString& text);
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    Q_INVOKABLE void open_playlist_detail(const QString& name);

protected:
    void keyPressEvent(QKeyEvent* event) override;
#if defined(Q_OS_WIN)
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private:
    void setup_ui();
    void connect_signals();
    void switch_page(int index);
    void apply_theme(const QString& theme);
    void check_updates();

    player::AudioPipeline m_pipeline;
    player::Db m_db;
    player::DlMgr m_dl;
    player::DiscordRPC m_discord_rpc;

    TitleBar* m_title_bar{nullptr};
    Sidebar* m_sidebar{nullptr};
    PlayerControls* m_controls{nullptr};
    QStackedWidget* m_stacked_widget{nullptr};

    LibraryView* m_library_view{nullptr};
    PlaylistsView* m_playlists_view{nullptr};
    DiscoverView* m_discover_view{nullptr};
    DownloadsView* m_downloads_view{nullptr};
    SettingsView* m_settings_view{nullptr};
    PlaylistView* m_playlist_view{nullptr};

    QString m_active_play_source;
};

}
