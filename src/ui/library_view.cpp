#include "library_view.hpp"
#include <QHeaderView>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QKeyEvent>
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
    add_file_btn->setFocusPolicy(Qt::NoFocus);
    connect(add_file_btn, &QPushButton::clicked, this, [this]() {
        QStringList file_paths = QFileDialog::getOpenFileNames(this, "Add Local Tracks", "", "Audio Files (*.mp3 *.flac *.wav *.m4a *.ogg)");
        for (const QString& file_path : file_paths) {
            if (!file_path.isEmpty()) {
                player::Track track = player::Track::extract_meta(file_path);
                m_db->add_track(track);
            }
        }
    });
    top_layout->addWidget(add_file_btn);
    main_layout->addLayout(top_layout);

    m_search_bar = new QLineEdit(this);
    m_search_bar->setPlaceholderText("Search tracks... (Ctrl + F to hide)");
    m_search_bar->setVisible(false);
    m_search_bar->setStyleSheet(R"(
        QLineEdit {
            background-color: #121212; border: 1px solid #1a1a1a; border-radius: 6px; padding: 8px 12px; color: #ffffff; font-size: 13px;
        }
    )");
    connect(m_search_bar, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_model->set_search_query(text);
        emit queue_updated(current_queue());
    });
    main_layout->addWidget(m_search_bar);

    m_table = new QTableView(this);
    m_model = new TrackModel(this);
    m_table->setModel(m_model);

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
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(0, Qt::AscendingOrder);

    m_delegate = new TrackDelegate(this);
    m_table->setItemDelegateForColumn(1, m_delegate);

    main_layout->addWidget(m_table);

    connect(m_table, &QTableView::clicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) return;
        int row = index.row();
        int col = index.column();
        if (col == 3) {
            auto track_id_str = m_model->data(m_model->index(row, 0), Qt::UserRole).toString();
            auto id = QUuid::fromString(track_id_str);
            m_db->toggle_favorite(id);
        } else {
            emit play_requested(current_queue(), static_cast<size_t>(row));
        }
    });

    connect(m_table->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, [this]() {
        emit queue_updated(current_queue());
    });

    connect(m_table, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QModelIndex index = m_table->indexAt(pos);
        if (!index.isValid()) return;

        int row = index.row();
        const auto& track = m_model->get_track(row);

        auto* menu = new QMenu(this);
        menu->setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a;");

        auto* play_act = menu->addAction("Play");
        auto* edit_act = menu->addAction("Edit Metadata");
        auto* delete_act = menu->addAction("Delete");

        QAction* selected = menu->exec(m_table->viewport()->mapToGlobal(pos));
        if (selected == play_act) {
            emit play_requested(current_queue(), static_cast<size_t>(row));
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

                if (!new_cover_path.isEmpty()) {
                    icons::utils::clear_cached_cover(track.file_path);
                }
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
    m_sub_label->setText(QString("%1 songs • stored on this device").arg(tracks.size()));
    m_model->set_tracks(&tracks);
    m_table->setUpdatesEnabled(true);
    emit queue_updated(current_queue());
}

void LibraryView::handle_favorite_changed(const QUuid&, bool) {
    refresh();
}

void LibraryView::toggle_search() {
    m_search_bar->setVisible(!m_search_bar->isVisible());
    if (m_search_bar->isVisible()) {
        m_search_bar->setFocus();
    } else {
        m_search_bar->clear();
    }
}

std::vector<player::Track> LibraryView::current_queue() const {
    std::vector<player::Track> queue;
    queue.reserve(m_model->rowCount());
    for (int i = 0; i < m_model->rowCount(); ++i) {
        queue.push_back(m_model->get_track(i));
    }
    return queue;
}

void LibraryView::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_F) {
        toggle_search();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

}
