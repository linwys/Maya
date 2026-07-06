#include "discover_view.hpp"
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include "search_overlay.hpp"
#include "icons.hpp"

namespace ui {

DiscoverView::DiscoverView(player::DlMgr* dl, QWidget* parent)
    : QWidget(parent), m_dl(dl) {
    setup_ui();
}

void DiscoverView::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(32, 32, 32, 32);
    main_layout->setSpacing(24);

    auto* top_layout = new QHBoxLayout();
    
    m_search_input = new QLineEdit(this);
    m_search_input->setPlaceholderText("Search tracks, playlists or paste link...");
    m_search_input->setStyleSheet(R"(
        QLineEdit {
            background-color: #121212;
            border: 1px solid #1a1a1a;
            border-radius: 18px;
            padding: 8px 16px;
            color: #ffffff;
            font-size: 14px;
        }
    )");
    connect(m_search_input, &QLineEdit::returnPressed, this, &DiscoverView::execute_search);
    top_layout->addWidget(m_search_input, 1);

    auto* search_btn = new QPushButton("Search", this);
    search_btn->setFixedSize(100, 36);
    search_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff; color: #000000; border-radius: 18px; font-weight: bold; font-size: 13px;
        }
        QPushButton:hover { background-color: #e6e6e6; }
    )");
    connect(search_btn, &QPushButton::clicked, this, &DiscoverView::execute_search);
    top_layout->addWidget(search_btn);

    m_dl_all_btn = new QPushButton("Download All", this);
    m_dl_all_btn->setFixedSize(120, 36);
    m_dl_all_btn->setFocusPolicy(Qt::NoFocus);
    m_dl_all_btn->setVisible(false);
    m_dl_all_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 18px; font-weight: bold; font-size: 13px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(m_dl_all_btn, &QPushButton::clicked, this, [this]() {
        emit batch_download_requested(m_last_results);
    });
    top_layout->addWidget(m_dl_all_btn);

    main_layout->addLayout(top_layout);

    m_results_table = new QTableWidget(this);
    m_results_table->setColumnCount(5);
    m_results_table->setHorizontalHeaderLabels({"ARTWORK", "TITLE", "ARTIST", "DURATION", "ACTION"});
    m_results_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_results_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_results_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_results_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_results_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_results_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    
    m_results_table->setColumnWidth(0, 55);
    m_results_table->setColumnWidth(2, 200);
    m_results_table->setColumnWidth(3, 80);
    m_results_table->setColumnWidth(4, 130);

    m_results_table->verticalHeader()->setVisible(false);
    m_results_table->verticalHeader()->setDefaultSectionSize(60);
    m_results_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_results_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_results_table->setFocusPolicy(Qt::NoFocus);
    m_results_table->setShowGrid(false);

    m_results_table->setStyleSheet(R"(
        QTableWidget {
            background-color: #0c0c0c;
            border: none;
            color: #ffffff;
            font-size: 13px;
        }
        QTableWidget::item {
            padding: 12px;
            border-bottom: 1px solid #141414;
        }
        QHeaderView::section {
            background-color: #0c0c0c;
            color: #8c8c8c;
            border: none;
            font-weight: bold;
            font-size: 11px;
            padding: 8px;
        }
    )");

    main_layout->addWidget(m_results_table);
}

void DiscoverView::execute_search() {
    QString query = m_search_input->text().trimmed();
    if (query.isEmpty()) return;

    m_results_table->setRowCount(0);
    m_dl_all_btn->setVisible(false);

    auto* overlay = new SearchOverlay(this->window());
    connect(overlay, &SearchOverlay::cancel_requested, this, [this, overlay]() {
        m_dl->cancel_search();
        overlay->hide_overlay();
        overlay->deleteLater();
    });
    overlay->show_overlay(QString("Parsing link / search query..."));

    auto process_results = [this, overlay](std::vector<player::WebTrack> results, QString err_msg) {
        overlay->hide_overlay();
        overlay->deleteLater();

        if (!err_msg.isEmpty()) {
            QMessageBox::critical(this->window(), "Search Error", 
                QString("Search / link parsing failed.\n\nReason:\n%1")
                .arg(err_msg)
            );
            return;
        }

        m_last_results = results;
        m_results_table->setRowCount(0);
        
        if (!results.empty()) {
            m_dl_all_btn->setVisible(true);
        }

        int count = static_cast<int>(results.size());
        for (int i = 0; i < count; ++i) {
            size_t idx = static_cast<size_t>(i);
            m_results_table->insertRow(i);

            auto* artwork_label = new QLabel(m_results_table);
            artwork_label->setFixedSize(48, 48);
            artwork_label->setStyleSheet("background-color: #1a1a1a; border-radius: 4px;");
            artwork_label->setScaledContents(true);
            m_results_table->setCellWidget(i, 0, artwork_label);

            auto* nam = new QNetworkAccessManager(this);
            connect(nam, &QNetworkAccessManager::finished, this, [artwork_label](QNetworkReply* reply) {
                if (reply->error() == QNetworkReply::NoError) {
                    QPixmap pixmap;
                    pixmap.loadFromData(reply->readAll());
                    if (!pixmap.isNull()) {
                        artwork_label->setPixmap(icons::utils::crop_to_square(pixmap, 48));
                    }
                }
                reply->deleteLater();
            });
            nam->get(QNetworkRequest(QUrl(results[idx].thumbnail_url)));

            m_results_table->setItem(i, 1, new QTableWidgetItem(results[idx].title));
            
            auto* artist_item = new QTableWidgetItem(results[idx].artist);
            artist_item->setForeground(QColor("#8c8c8c"));
            m_results_table->setItem(i, 2, artist_item);

            int secs = static_cast<int>(results[idx].duration.count());
            auto* dur_item = new QTableWidgetItem(QString::asprintf("%d:%02d", secs / 60, secs % 60));
            dur_item->setForeground(QColor("#8c8c8c"));
            m_results_table->setItem(i, 3, dur_item);

            auto* dl_btn = new QPushButton("Download", this);
            dl_btn->setFixedSize(110, 30);
            dl_btn->setFocusPolicy(Qt::NoFocus);
            dl_btn->setStyleSheet(R"(
                QPushButton {
                    background-color: #1a1a1a; color: #ffffff; border-radius: 15px; font-weight: bold; font-size: 12px; border: 1px solid #333333;
                    outline: none;
                }
                QPushButton:hover { background-color: #262626; }
            )");
            
            connect(dl_btn, &QPushButton::clicked, this, [this, idx]() {
                emit download_requested(m_last_results[idx]);
            });

            m_results_table->setCellWidget(i, 4, dl_btn);
        }
    };

    if (query.startsWith("http://") || query.startsWith("https://")) {
        m_dl->parse_online_playlist(query, process_results);
    } else {
        m_dl->search_online(query, process_results);
    }
}

}