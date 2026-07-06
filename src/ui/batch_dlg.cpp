/* old | not used
#include "batch_dlg.hpp"
#include <QMouseEvent>
#include <QListWidgetItem>

namespace ui {

BatchDlg::BatchDlg(
    const std::vector<player::WebTrack>& tracks, 
    player::DlMgr* dl, //ss0s
    player::Db* db, 
    QWidget* parent
) : QDialog(parent), m_tracks(tracks), m_dl(dl), m_db(db) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(500, 420);
    setStyleSheet("background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* header_layout = new QHBoxLayout();
    auto* header_lbl = new QLabel("Batch Download Queue", this);
    header_lbl->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    header_layout->addWidget(header_lbl);
    header_layout->addStretch();
    
    m_close_btn = new QPushButton("Close", this);
    m_close_btn->setFixedSize(70, 26);
    m_close_btn->setFocusPolicy(Qt::NoFocus);
    m_close_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 13px; font-weight: bold; font-size: 11px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(m_close_btn, &QPushButton::clicked, this, &QDialog::accept);
    header_layout->addWidget(m_close_btn);
    layout->addLayout(header_layout);

    m_track_list = new QListWidget(this);
    m_track_list->setFocusPolicy(Qt::NoFocus);
    m_track_list->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a1a; border: 1px solid #1a1a1a; border-radius: 8px; color: #ffffff; padding: 4px;
        }
        QListWidget::item { padding: 8px; border-bottom: 1px solid #141414; }
    )");

    for (const auto& track : m_tracks) {
        m_track_list->addItem(QString("%1 — %2").arg(track.title).arg(track.artist));
    }
    layout->addWidget(m_track_list);

    m_status_label = new QLabel("Ready to start.", this);
    m_status_label->setStyleSheet("font-size: 13px; color: #8c8c8c; border: none; background: transparent;");
    m_status_label->setWordWrap(true);
    layout->addWidget(m_status_label);

    m_progress_bar = new QProgressBar(this);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setValue(0);
    m_progress_bar->setFixedHeight(6);
    m_progress_bar->setTextVisible(false);
    m_progress_bar->setStyleSheet(R"(
        QProgressBar { background-color: #262626; border: none; border-radius: 3px; }
        QProgressBar::chunk { background-color: #ffffff; border-radius: 3px; }
    )");
    layout->addWidget(m_progress_bar);

    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(12);

    m_skip_btn = new QPushButton("Skip", this);
    m_skip_btn->setFixedHeight(36);
    m_skip_btn->setFocusPolicy(Qt::NoFocus);
    m_skip_btn->setVisible(false);
    m_skip_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 18px; font-weight: bold; font-size: 13px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(m_skip_btn, &QPushButton::clicked, this, [this]() {
        m_skip_btn->setVisible(false);
        m_failed_state = false;
        m_current_index++;
        start_current_download();
    });
    btn_layout->addWidget(m_skip_btn);

    m_action_btn = new QPushButton("Start Download", this);
        m_action_btn->setFixedHeight(36);
        m_action_btn->setFocusPolicy(Qt::NoFocus);
    m_action_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff; color: #000000; border-radius: 18px; font-weight: bold; font-size: 13px; border: none;
        }
        QPushButton:hover { background-color: #e6e6e6; }
    )");
    connect(m_action_btn, &QPushButton::clicked, this, &BatchDlg::handle_action_button);
    btn_layout->addWidget(m_action_btn);

    layout->addLayout(btn_layout);

    connect(m_dl, &player::DlMgr::progress_changed, this, [this](const QString& id, double percent) {
        if (m_current_index < m_tracks.size() && m_tracks[m_current_index].id == id) {
            m_progress_bar->setValue(static_cast<int>(percent));
        }
    });
}

void BatchDlg::start_current_download() {
    if (m_current_index >= m_tracks.size()) {
        m_status_label->setText("All downloads completed!");
        m_action_btn->setText("Done");
        m_action_btn->setEnabled(true);
        m_skip_btn->setVisible(false);
        return;
    }

    m_failed_state = false;
    m_skip_btn->setVisible(false);
    m_action_btn->setEnabled(false);
    m_progress_bar->setValue(0);

    for (int i = 0; i < m_track_list->count(); ++i) {
        auto* item = m_track_list->item(i);
        if (static_cast<size_t>(i) == m_current_index) {
            item->setSelected(true);
            m_track_list->scrollToItem(item);
        } else {
            item->setSelected(false);
        }
    }

    const auto& track = m_tracks[m_current_index];
    m_status_label->setText(QString("Downloading: %1...").arg(track.title));

    m_dl->download(track, "Downloads", m_db->save_format(), [this](bool ok, QString err, player::Track downloaded) {
        if (ok) {
            m_db->add_track(downloaded);
            auto* item = m_track_list->item(static_cast<int>(m_current_index));
            if (item) {
                item->setText(item->text() + " [Saved]");
                item->setForeground(QColor("#ff4d4d"));
            }
            m_status_label->setText("Download complete. Click Next to continue");
            m_action_btn->setText("Next");
            m_action_btn->setEnabled(true);
        } else {
            m_failed_state = true;
            m_status_label->setText(QString("Error: %1").arg(err));
            m_action_btn->setText("Try Again");
            m_action_btn->setEnabled(true);
            m_skip_btn->setVisible(true);
        }
    });
}

void BatchDlg::handle_action_button() {
    if (m_action_btn->text() == "Done") {
        accept();
        return;
    }

    if (m_failed_state) {
        start_current_download();
    } else {
        if (m_action_btn->text() == "Next") {
            m_current_index++;
        }
        start_current_download();
    }
}

void BatchDlg::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drag_position = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void BatchDlg::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_drag_position);
        event->accept();
    }
}

} */ 