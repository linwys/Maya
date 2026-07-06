#include "library_view.hpp"
#include <QHeaderView>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QDir>
#include <bass.h>
#include "icons.hpp"
#include "metadata_editor.hpp"

namespace ui {
LibraryView::LibraryView(player::Db* db, QWidget* parent) 
    : QWidget(parent), m_db(db) {
    setup_ui();
    connect(m_db, &player::Db::library_changed, this, &LibraryView::refresh);
    connect(m_db, &player::Db::track_favorite_changed, this, &LibraryView::handle_favorite_changed);
    refresh();
}

void LibraryView::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(32, 32, 32, 32);
    main_layout->setSpacing(24);

    auto* top_layout = new QHBoxLayout();
    
    auto* text_layout = new QVBoxLayout();
    m_header_label = new QLabel("All Tracks", this);
    m_header_label->setStyleSheet("font-size: 28px; font-weight: bold;");
    m_sub_label = new QLabel("0 songs", this);
    m_sub_label->setStyleSheet("font-size: 14px;");
    text_layout->addWidget(m_header_label);
    text_layout->addWidget(m_sub_label);
    top_layout->addLayout(text_layout);
    top_layout->addStretch();

    auto* add_file_btn = new QPushButton("+ Add File", this);
    add_file_btn->setFixedSize(110, 36);
    connect(add_file_btn, &QPushButton::clicked, this, [this]() {
        QString file_path = QFileDialog::getOpenFileName(this, "Add Local Track", "", "Audio Files (*.mp3 *.flac *.wav *.m4a *.ogg)");
        if (!file_path.isEmpty()) {
            player::Track track;
            track.id = QUuid::createUuid();
            track.file_path = file_path;
            
            QString native_path = QDir::toNativeSeparators(file_path);

#if defined(Q_OS_WIN)
            const void* file_ptr = native_path.utf16();
#else
            QByteArray path_utf8 = native_path.toUtf8();
            const void* file_ptr = path_utf8.constData();
#endif

            unsigned long temp_stream = BASS_StreamCreateFile(FALSE, file_ptr, 0, 0, BASS_STREAM_DECODE | BASS_UNICODE);
            if (temp_stream) {
                double secs = BASS_ChannelBytes2Seconds(temp_stream, BASS_ChannelGetLength(temp_stream, BASS_POS_BYTE));
                track.duration = std::chrono::seconds(static_cast<int64_t>(secs));
                BASS_StreamFree(temp_stream);
            } else {
                track.duration = std::chrono::seconds(0);
            }
            
            QFileInfo file_info(file_path);
            track.title = file_info.completeBaseName();
            track.artist = "Local Artist";
            track.album = "None";
            track.is_local = true;
            m_db->add_track(track);
        }
    });
    top_layout->addWidget(add_file_btn);
    main_layout->addLayout(top_layout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    
    auto* header_hash = new QTableWidgetItem("#");
    auto* header_title = new QTableWidgetItem("TITLE");
    auto* header_album = new QTableWidgetItem("ALBUM");
    auto* header_liked = new QTableWidgetItem("");
    
    auto* header_time = new QTableWidgetItem();
    header_time->setIcon(icons::from_svg(icons::clock, QColor("#8c8c8c")));

    m_table->setHorizontalHeaderItem(0, header_hash);
    m_table->setHorizontalHeaderItem(1, header_title);
    m_table->setHorizontalHeaderItem(2, header_album);
    m_table->setHorizontalHeaderItem(3, header_liked);
    m_table->setHorizontalHeaderItem(4, header_time);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 55);
    m_table->setColumnWidth(2, 200);
    m_table->setColumnWidth(3, 60);
    m_table->setColumnWidth(4, 80);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(72);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    m_delegate = new TrackDelegate(this);
    m_table->setItemDelegateForColumn(1, m_delegate);

    main_layout->addWidget(m_table);

    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (col == 3) {
            auto* item = m_table->item(row, 0);
            if (item) {
                auto id = QUuid::fromString(item->data(Qt::UserRole).toString());
                m_db->toggle_favorite(id);
            }
        } else if (col != 3) {
            const auto& tracks = m_db->tracks();
            if (row >= 0 && row < static_cast<int>(tracks.size())) {
                emit play_requested(tracks, static_cast<size_t>(row));
            }
        }
    });

    connect(m_table, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem* item = m_table->itemAt(pos);
        if (!item) return;

        int row = item->row();
        const auto& tracks = m_db->tracks();
        if (row < 0 || row >= static_cast<int>(tracks.size())) return;

        const auto& track = tracks[static_cast<size_t>(row)];

        auto* menu = new QMenu(this);
        menu->setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a;");

        auto* play_act = menu->addAction("Play");
        auto* edit_act = menu->addAction("Edit Metadata");
        auto* delete_act = menu->addAction("Delete");

        QAction* selected = menu->exec(m_table->viewport()->mapToGlobal(pos));
        if (selected == play_act) {
            emit play_requested(tracks, static_cast<size_t>(row));
        } else if (selected == edit_act) {
            MetadataEditorDialog dialog(track, this);
            if (dialog.exec() == QDialog::Accepted) {
                QString new_cover_path;
                QString new_cover = dialog.cover_path();
                if (!new_cover.isEmpty()) {
                    QString base = track.file_path;
                    int dot = base.lastIndexOf('.');
                    if (dot != -1) {
                        base.truncate(dot);
                    }
                    QString dest_png = base + ".png";
                    QFile::remove(dest_png);
                    if (QFile::copy(new_cover, dest_png)) {
                        new_cover_path = dest_png;
                    }
                }

                m_db->update_track(
                    track.id, 
                    dialog.title(), 
                    dialog.artist(), 
                    dialog.album().isEmpty() ? "None" : dialog.album(), 
                    new_cover_path
                );
            }
        } else if (selected == delete_act) {
            m_db->remove_track(track.id);
        }
        menu->deleteLater();
    });
}

void LibraryView::refresh() {
    m_table->setUpdatesEnabled(false);

    const auto& tracks = m_db->tracks();
    int count = static_cast<int>(tracks.size());
    m_sub_label->setText(QString("%1 songs • stored on this device").arg(count));
    
    m_table->setRowCount(0);
    m_table->setRowCount(count);

    for (int i = 0; i < count; ++i) {
        auto* num_item = new QTableWidgetItem(QString::asprintf("%02d", i + 1));
        num_item->setData(Qt::UserRole, tracks[i].id.toString());
        m_table->setItem(i, 0, num_item);

        size_t idx = static_cast<size_t>(i);
        auto* title_item = new QTableWidgetItem(tracks[idx].title);
        title_item->setData(Qt::UserRole, tracks[idx].artist);
        title_item->setData(Qt::UserRole + 1, tracks[idx].file_path);
        m_table->setItem(i, 1, title_item);

        auto* album_item = new QTableWidgetItem(tracks[idx].album);
        m_table->setItem(i, 2, album_item);

        auto* liked_item = new QTableWidgetItem();
        QIcon heart_icon = tracks[idx].is_favorite ? 
            icons::from_svg(icons::heart_filled, QColor("#ff4d4d")) : 
            icons::from_svg(icons::heart, QColor("#8c8c8c"));
        liked_item->setIcon(heart_icon);
        liked_item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 3, liked_item);

        int secs = static_cast<int>(tracks[idx].duration.count());
        auto* dur_item = new QTableWidgetItem(QString::asprintf("%d:%02d", secs / 60, secs % 60));
        m_table->setItem(i, 4, dur_item);
    }

    m_table->setUpdatesEnabled(true);
}

void LibraryView::handle_favorite_changed(const QUuid& id, bool is_favorite) {
    QString id_str = id.toString();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto* item = m_table->item(row, 0);
        if (item && item->data(Qt::UserRole).toString() == id_str) {
            auto* liked_item = m_table->item(row, 3);
            if (liked_item) {
                QIcon heart_icon = is_favorite ? 
                    icons::from_svg(icons::heart_filled, QColor("#ff4d4d")) : 
                    icons::from_svg(icons::heart, QColor("#8c8c8c"));
                liked_item->setIcon(heart_icon);
            }
            break;
        }
    }
}

}