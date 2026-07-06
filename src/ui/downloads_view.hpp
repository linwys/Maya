#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <map>
#include "player/dl_mgr.hpp"

namespace ui {

class DownloadsView : public QWidget {
    Q_OBJECT
public:
    explicit DownloadsView(player::DlMgr* dl, QWidget* parent = nullptr);
    ~DownloadsView() override = default;

    void add_active_download(const player::WebTrack& track);

private:
    struct DownloadItem {
        QWidget* widget;
        QProgressBar* bar;
        QLabel* status_label;
        QPushButton* retry_btn;
    };

    void setup_ui();

    player::DlMgr* m_dl{nullptr};
    QVBoxLayout* m_scroll_layout{nullptr};
    std::map<QString, DownloadItem> m_items;
};

}