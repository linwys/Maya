#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "player/dl_mgr.hpp"

namespace ui {

class DiscoverView : public QWidget {
    Q_OBJECT
public:
    explicit DiscoverView(player::DlMgr* dl, QWidget* parent = nullptr);
    ~DiscoverView() override = default;

signals:
    void download_requested(const player::WebTrack& track);
    void batch_download_requested(const std::vector<player::WebTrack>& tracks);

private:
    void setup_ui();
    void execute_search();

    player::DlMgr* m_dl{nullptr};
    QLineEdit* m_search_input{nullptr};
    QTableWidget* m_results_table{nullptr};
    QPushButton* m_dl_all_btn{nullptr};
    std::vector<player::WebTrack> m_last_results;
};

}