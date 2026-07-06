#include "playlists_view.hpp"
#include <QFrame>
#include <QFile>
#include <QMenu>
#include <QHash>
#include "playlist_dialog.hpp"
#include "icons.hpp"
#include "main_window.hpp"

namespace ui {

PlaylistsView::PlaylistsView(player::Db* db, QWidget* parent) 
    : QWidget(parent), m_db(db) {
    setup_ui();
    connect(m_db, &player::Db::playlists_changed, this, &PlaylistsView::refresh);
    refresh();
}

void PlaylistsView::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(32, 32, 32, 32);
    main_layout->setSpacing(24);

    auto* top_layout = new QHBoxLayout();
    auto* header_label = new QLabel("Playlists", this);
    header_label->setStyleSheet("font-size: 28px; font-weight: bold;");
    top_layout->addWidget(header_label);
    top_layout->addStretch();

    auto* create_btn = new QPushButton("+ New Playlist", this);
    create_btn->setFixedSize(120, 36);
    connect(create_btn, &QPushButton::clicked, this, [this]() {
        PlaylistDialog dialog(m_db, this);
        if (dialog.exec() == QDialog::Accepted) {
            QString name = dialog.playlist_name();
            if (!name.isEmpty()) {
                m_db->create_playlist(name, dialog.cover_path());
                m_db->add_to_playlist_batch(name, dialog.selected_tracks());
            }
        }
    });

    top_layout->addWidget(create_btn);
    main_layout->addLayout(top_layout);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("background-color: transparent; border: none;");

    auto* container = new QWidget(scroll);
    container->setStyleSheet("background-color: transparent;");
    
    auto* container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(0, 0, 0, 0);
    container_layout->setSpacing(0);

    m_grid_layout = new QGridLayout();
    m_grid_layout->setSpacing(24);
    container_layout->addLayout(m_grid_layout);
    container_layout->addStretch();

    scroll->setWidget(container);
    main_layout->addWidget(scroll);
}

void PlaylistsView::refresh() {
    m_grid_layout->setEnabled(false);

    QLayoutItem* item;
    while ((item = m_grid_layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    const auto& playlists = m_db->playlists();
    const auto& all_tracks = m_db->tracks();

    QHash<QUuid, const player::Track*> track_lookup;
    track_lookup.reserve(static_cast<qsizetype>(all_tracks.size()));
    for (const auto& track : all_tracks) {
        track_lookup.insert(track.id, &track);
    }

    int row = 0;
    int col = 0;

    for (const auto& [name, pl] : playlists) {
        auto* card = new QFrame(this);
        card->setFixedSize(220, 300);
        card->setObjectName("playlist_card");

        auto* card_layout = new QVBoxLayout(card);
        card_layout->setContentsMargins(16, 16, 16, 16);
        card_layout->setSpacing(8);

        auto* art_placeholder = new QLabel(card);
        art_placeholder->setFixedSize(188, 180);
        art_placeholder->setStyleSheet("background-color: #1a1a1a; border-radius: 8px;");
        art_placeholder->setScaledContents(true);

        QPixmap cover_pix;
        if (!pl.cover_path.isEmpty() && QFile::exists(pl.cover_path)) {
            cover_pix.load(pl.cover_path);
        } else if (!pl.track_ids.empty()) {
            QUuid first_id = pl.track_ids[0];
            if (track_lookup.contains(first_id)) {
                cover_pix = icons::utils::get_cached_cover(track_lookup.value(first_id)->file_path, 188);
            }
        }

        if (!cover_pix.isNull()) {
            art_placeholder->setPixmap(icons::utils::crop_to_square(cover_pix, 188));
        }

        card_layout->addWidget(art_placeholder);

        auto* name_label = new QLabel(name, card);
        name_label->setStyleSheet("font-size: 15px; font-weight: bold; background: transparent; border: none;");
        card_layout->addWidget(name_label);

        auto* count_label = new QLabel(QString("%1 tracks").arg(pl.track_ids.size()), card);
        count_label->setStyleSheet("font-size: 12px; color: #8c8c8c; background: transparent; border: none;");
        card_layout->addWidget(count_label);

        auto* overlay_btn = new QPushButton(card);
        overlay_btn->setGeometry(0, 0, 220, 300);
        overlay_btn->setStyleSheet("background: transparent; border: none;");
        overlay_btn->setCursor(Qt::PointingHandCursor);
        overlay_btn->setFocusPolicy(Qt::NoFocus);
        overlay_btn->setContextMenuPolicy(Qt::CustomContextMenu);
        
        connect(overlay_btn, &QPushButton::clicked, this, [this, name]() {
            auto* main_win = qobject_cast<MainWindow*>(this->window());
            if (main_win) {
                QMetaObject::invokeMethod(main_win, "open_playlist_detail", Qt::QueuedConnection, Q_ARG(QString, name));
            }
        });

        connect(overlay_btn, &QWidget::customContextMenuRequested, this, [this, name](const QPoint&) {
            auto* menu = new QMenu(this);
            menu->setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a;");
            
            auto* edit_act = menu->addAction("Edit Playlist");
            auto* delete_act = menu->addAction("Delete Playlist");
            
            QAction* selected = menu->exec(QCursor::pos());
            if (selected == edit_act) {
                const auto& inner_playlists = m_db->playlists();
                if (inner_playlists.contains(name)) {
                    const auto& current_pl = inner_playlists.at(name);
                    PlaylistDialog dialog(m_db, this);
                    dialog.set_playlist_data(current_pl.name, current_pl.cover_path, current_pl.track_ids);
                    if (dialog.exec() == QDialog::Accepted) {
                        m_db->update_playlist_data(name, dialog.cover_path(), dialog.selected_tracks());
                    }
                }
            } else if (selected == delete_act) {
                m_db->remove_playlist(name);
            }
            menu->deleteLater();
        });

        m_grid_layout->addWidget(card, row, col);

        col++;
        if (col >= 4) {
            col = 0;
            row++;
        }
    }

    m_grid_layout->setEnabled(true);
}

}