#include "downloads_view.hpp"
#include <QScrollArea>
#include <QHBoxLayout>

namespace ui {

DownloadsView::DownloadsView(player::DlMgr* dl, QWidget* parent)
    : QWidget(parent), m_dl(dl) {
    setup_ui();

    //0001
    connect(m_dl, &player::DlMgr::progress_changed, this, [this](const QString& track_id, double percent) {
        if (m_items.contains(track_id)) {
            m_items[track_id].bar->setValue(static_cast<int>(percent));
        }
    });
    //0002
    connect(m_dl, &player::DlMgr::download_started, this, [this](const QString& track_id) {
        if (m_items.contains(track_id)) {
            m_items[track_id].status_label->setText("DOWNLOADING");
            m_items[track_id].bar->setValue(0);
            m_items[track_id].bar->setVisible(true);
            m_items[track_id].retry_btn->setVisible(false);
        }
    });
//0003
    connect(m_dl, &player::DlMgr::download_completed, this, [this](const QString& track_id, const player::Track&) {
        if (m_items.contains(track_id)) {
            m_items[track_id].bar->setValue(100);
            m_items[track_id].status_label->setText("SAVED TO LIBRARY");
            m_items[track_id].retry_btn->setVisible(false);
        }
    });
    //0004
    connect(m_dl, &player::DlMgr::download_failed, this, [this](const QString& track_id, const QString& err) {
        if (m_items.contains(track_id)) {
            m_items[track_id].status_label->setText(QString("FAILED: %1").arg(err));
            m_items[track_id].bar->setVisible(false);
            m_items[track_id].retry_btn->setVisible(true);
        }
    });
    //0005
    connect(m_dl, &player::DlMgr::download_queued, this, [this](const player::WebTrack& track) {
        add_active_download(track);
        if (m_items.contains(track.id)) {
            m_items[track.id].status_label->setText("QUEUED");
            m_items[track.id].bar->setVisible(true);
            m_items[track.id].retry_btn->setVisible(false);
        }
    });
}

void DownloadsView::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(32, 32, 32, 32);
    main_layout->setSpacing(24);

    auto* header_layout = new QHBoxLayout();
    auto* title_label = new QLabel("Downloads", this);
    title_label->setStyleSheet("font-size: 28px; font-weight: bold; color: #ffffff;");
    header_layout->addWidget(title_label);
    header_layout->addStretch();
    main_layout->addLayout(header_layout);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet("background-color: #0c0c0c; border: none;");

    auto* container = new QWidget(scroll);
    container->setStyleSheet("background-color: #0c0c0c;");
    m_scroll_layout = new QVBoxLayout(container);
    m_scroll_layout->setContentsMargins(0, 0, 0, 0);
    m_scroll_layout->setSpacing(12);
    m_scroll_layout->addStretch();

    scroll->setWidget(container);
    main_layout->addWidget(scroll);
}

void DownloadsView::add_active_download(const player::WebTrack& track) {
    if (m_items.contains(track.id)) return;

    auto* widget = new QWidget(this);
    widget->setFixedHeight(80);
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    widget->setStyleSheet(R"(
        QWidget {
            background-color: #121212;
            border-radius: 8px;
            border: 1px solid #1a1a1a;
        }
    )");

    auto* item_layout = new QVBoxLayout(widget);
    item_layout->setContentsMargins(16, 12, 16, 12);

    auto* text_layout = new QHBoxLayout();
    auto* title_lbl = new QLabel(QString("%1 — %2").arg(track.title).arg(track.artist), widget);
    title_lbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    title_lbl->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    
    auto* status_lbl = new QLabel("QUEUED", widget);
    status_lbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #8c8c8c; border: none; background: transparent;");

    auto* retry_btn = new QPushButton("Try again", widget);
    retry_btn->setFixedSize(80, 24);
    retry_btn->setFocusPolicy(Qt::NoFocus);
    retry_btn->setVisible(false);
    retry_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 12px; font-weight: bold; font-size: 11px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");

    QString track_id = track.id;
    connect(retry_btn, &QPushButton::clicked, this, [this, track_id]() {
        m_dl->retry_download(track_id);
    });

    text_layout->addWidget(title_lbl, 1);
    text_layout->addWidget(status_lbl);
    text_layout->addWidget(retry_btn);
    item_layout->addLayout(text_layout);

    auto* bar = new QProgressBar(widget);
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setFixedHeight(4);
    bar->setTextVisible(false);
    bar->setStyleSheet(R"(
        QProgressBar {
            background-color: #262626;
            border: none;
            border-radius: 2px;
        }
        QProgressBar::chunk {
            background-color: #ffffff;
            border-radius: 2px;
        }
    )");
    item_layout->addWidget(bar);

    int insert_pos = m_scroll_layout->count() - 1;
    if (insert_pos < 0) {
        insert_pos = 0;
    }
    m_scroll_layout->insertWidget(insert_pos, widget);

    m_items[track.id] = DownloadItem{ widget, bar, status_lbl, retry_btn };
}

}