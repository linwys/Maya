/* old | not used
#pragma once 
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "player/dl_mgr.hpp"
#include "player/db.hpp"

namespace ui {

class BatchDlg : public QDialog {
    Q_OBJECT
public:
    explicit BatchDlg(const std::vector<player::WebTrack>& tracks, player::DlMgr* dl, player::Db* db, QWidget* parent = nullptr);
    ~BatchDlg() override = default;

private:
    void start_current_download(); 
    void handle_action_button();            

    std::vector<player::WebTrack> m_tracks;
    player::DlMgr* m_dl{nullptr};
    player::Db* m_db{nullptr};
    size_t m_current_index{0};

    QLabel* m_status_label{nullptr};
    QProgressBar* m_progress_bar{nullptr};
    QListWidget* m_track_list{nullptr};
    QPushButton* m_action_btn{nullptr};
    QPushButton* m_skip_btn{nullptr};
    QPushButton* m_close_btn{nullptr};
    bool m_failed_state{false};
    QPoint m_drag_position;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
};

} 
*/