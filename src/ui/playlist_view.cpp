#include "playlist_view.hpp"
#include <QHeaderView>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QPainter>
#include <QHash>
#include <QApplication>
#include "icons.hpp"

namespace ui {

void TrackDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    if (auto* style = opt.widget ? opt.widget->style() : QApplication::style()) {
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QString title = index.data(Qt::DisplayRole).toString();
    QString artist = index.data(Qt::UserRole).toString();
    QString file_path = index.data(Qt::UserRole + 1).toString();
    int img_size = 52;
    int x = opt.rect.x() + 8;
    int y = opt.rect.y() + (opt.rect.height() - img_size) / 2;
    QPixmap pix = ui::icons::utils::get_cached_cover(file_path, img_size);
    if (!pix.isNull()) {
        painter->drawPixmap(x, y, pix);
    } else {
        painter->fillRect(x, y, img_size, img_size, QColor("#1a1a1a"));
    }

    int tx = x + img_size + 12;
    int ty = opt.rect.y() + (opt.rect.height() - 40) / 2;
    
    //draw track title
    painter->setPen(opt.palette.text().color());
    if (opt.state & QStyle::State_Selected) {
        painter->setPen(opt.palette.highlightedText().color());
    }
    QFont tf = opt.font;
    tf.setBold(true);
    tf.setPointSize(10);
    painter->setFont(tf);
    painter->drawText(QRect(tx, ty, opt.rect.width() - tx - 8, 20), Qt::AlignLeft | Qt::AlignVCenter, title);

     //draw track artist
    QColor muted_color = opt.state & QStyle::State_Selected ? opt.palette.highlightedText().color() : QColor("#8c8c8c");
    painter->setPen(muted_color);
    QFont af = opt.font;
    af.setPointSize(8);
    painter->setFont(af);
    painter->drawText(QRect(tx, ty + 20, opt.rect.width() - tx - 8, 16), Qt::AlignLeft | Qt::AlignVCenter, artist);

    painter->restore();
}

QSize TrackDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return QSize(200, 72);
}

void PlaylistHeader::set_data(const QPixmap& cover, const QColor& theme_bg) {
    m_cover = cover;
    m_theme_bg = theme_bg;
    update();
}

void PlaylistHeader::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    painter.fillRect(rect(), m_theme_bg);

    if (!m_cover.isNull()) {
        painter.setOpacity(0.45);
        QPixmap scaled = m_cover.scaled(width(), width(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int y = (height() - scaled.height()) / 2;
        painter.drawPixmap(0, y, scaled);

        painter.setOpacity(1.0);
        QLinearGradient hor_gradient(0, 0, width(), 0);
        hor_gradient.setColorAt(0.0, Qt::transparent);
        hor_gradient.setColorAt(0.4, QColor(m_theme_bg.red(), m_theme_bg.green(), m_theme_bg.blue(), 100));
        hor_gradient.setColorAt(0.7, QColor(m_theme_bg.red(), m_theme_bg.green(), m_theme_bg.blue(), 220));
        hor_gradient.setColorAt(1.0, m_theme_bg);
        painter.fillRect(rect(), hor_gradient);

        QLinearGradient vert_gradient(0, height() - 80, 0, height());
        vert_gradient.setColorAt(0.0, Qt::transparent);
        vert_gradient.setColorAt(1.0, m_theme_bg);
        painter.fillRect(rect(), vert_gradient);
    }
}

PlaylistView::PlaylistView(player::Db* db, QWidget* parent)
    : QWidget(parent), m_db(db) {
    setup_ui();
    connect(m_db, &player::Db::library_changed, this, &PlaylistView::refresh);
    
    connect(m_db, &player::Db::playlists_changed, this, [this]() {
        if (m_playlist_name == "Favourites") {
            refresh();
        }
    });
    connect(m_db, &player::Db::track_favorite_changed, this, &PlaylistView::handle_favorite_changed);
}

void PlaylistView::setup_ui() {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    m_header_widget = new PlaylistHeader(this);
    m_header_widget->setObjectName("playlist_header");

    auto* header_layout = new QHBoxLayout(m_header_widget);
    header_layout->setContentsMargins(32, 32, 32, 32);
    header_layout->setSpacing(24);

    m_cover_label = new QLabel(m_header_widget);
    m_cover_label->setFixedSize(140, 140);
    m_cover_label->setStyleSheet("background-color: #1a1a1a; border-radius: 8px;");
    m_cover_label->setScaledContents(true);
    header_layout->addWidget(m_cover_label);

    auto* text_layout = new QVBoxLayout();
    text_layout->setSpacing(4);
    
    auto* type_lbl = new QLabel("PLAYLIST", m_header_widget);
    type_lbl->setStyleSheet("font-size: 11px; font-weight: bold; color: #8c8c8c; letter-spacing: 1px;");
    text_layout->addWidget(type_lbl);

    m_title_label = new QLabel(m_header_widget);
    m_title_label->setStyleSheet("font-size: 32px; font-weight: bold; color: #ffffff;");
    text_layout->addWidget(m_title_label);

    m_sub_label = new QLabel(m_header_widget);
    m_sub_label->setStyleSheet("font-size: 13px; color: #8c8c8c;");
    text_layout->addWidget(m_sub_label);
    
    header_layout->addLayout(text_layout, 1);
    main_layout->addWidget(m_header_widget);

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
            if (row >= 0 && row < static_cast<int>(m_tracks.size())) {
                emit play_requested(m_tracks, static_cast<size_t>(row));
            }
        }
    });

    connect(m_table, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTableWidgetItem* item = m_table->itemAt(pos);
        if (!item) return;

        int row = item->row();
        if (row < 0 || row >= static_cast<int>(m_tracks.size())) return;

        const auto& track = m_tracks[static_cast<size_t>(row)];

        auto* menu = new QMenu(this);
        menu->setStyleSheet("background-color: #121212; color: #ffffff; border: 1px solid #1a1a1a;");

        auto* play_act = menu->addAction("Play");
        auto* remove_act = menu->addAction("Remove from Playlist");

        QAction* selected = menu->exec(m_table->viewport()->mapToGlobal(pos));
        if (selected == play_act) {
            emit play_requested(m_tracks, static_cast<size_t>(row));
        } else if (selected == remove_act) {
            m_db->remove_from_playlist(m_playlist_name, track.id);
        }
        menu->deleteLater();
    });
}

void PlaylistView::open_playlist(const QString& name) {
    m_playlist_name = name;
    refresh();
}

void PlaylistView::set_theme_colors(const QColor& bg_color) {
    m_theme_bg = bg_color;
    m_header_widget->set_data(m_current_cover, m_theme_bg);
}

void PlaylistView::refresh() {
    m_tracks.clear();
    const auto& playlists = m_db->playlists();
    if (!playlists.contains(m_playlist_name)) return;

    const auto& current_pl = playlists.at(m_playlist_name);
    const auto& all_tracks = m_db->tracks();

    QHash<QUuid, const player::Track*> track_lookup;
    track_lookup.reserve(static_cast<qsizetype>(all_tracks.size()));
    for (const auto& track : all_tracks) {
        track_lookup.insert(track.id, &track);
    }

    m_tracks.reserve(current_pl.track_ids.size());
    for (const auto& id : current_pl.track_ids) {
        if (track_lookup.contains(id)) {
            m_tracks.push_back(*track_lookup.value(id));
        }
    }

    m_title_label->setText(m_playlist_name);
    m_sub_label->setText(QString("%1 tracks • added by you").arg(m_tracks.size()));

    QPixmap cover_pix;
    if (!current_pl.cover_path.isEmpty() && QFile::exists(current_pl.cover_path)) {
        cover_pix.load(current_pl.cover_path);
    } else if (!m_tracks.empty()) {
        cover_pix = icons::utils::get_cached_cover(m_tracks[0].file_path, 140);
    }

    m_current_cover = cover_pix;
    m_header_widget->set_data(m_current_cover, m_theme_bg);

    if (!cover_pix.isNull()) {
        m_cover_label->setPixmap(icons::utils::crop_to_square(cover_pix, 140));
    } else {
        m_cover_label->setPixmap(QPixmap());
    }

    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(0);
    
    int count = static_cast<int>(m_tracks.size());
    m_table->setRowCount(count);

    for (int i = 0; i < count; ++i) {
        auto* num_item = new QTableWidgetItem(QString::asprintf("%02d", i + 1));
        num_item->setData(Qt::UserRole, m_tracks[i].id.toString());
        m_table->setItem(i, 0, num_item);

        size_t idx = static_cast<size_t>(i);

        auto* title_item = new QTableWidgetItem(m_tracks[idx].title);
        title_item->setData(Qt::UserRole, m_tracks[idx].artist);
        title_item->setData(Qt::UserRole + 1, m_tracks[idx].file_path);
        m_table->setItem(i, 1, title_item);

        auto* album_item = new QTableWidgetItem(m_tracks[idx].album);
        m_table->setItem(i, 2, album_item);

        auto* liked_item = new QTableWidgetItem();
        QIcon heart_icon = m_tracks[idx].is_favorite ? 
            icons::from_svg(icons::heart_filled, QColor("#ff4d4d")) : 
            icons::from_svg(icons::heart, QColor("#8c8c8c"));
        liked_item->setIcon(heart_icon);
        liked_item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 3, liked_item);

        int secs = static_cast<int>(m_tracks[idx].duration.count());
        auto* dur_item = new QTableWidgetItem(QString::asprintf("%d:%02d", secs / 60, secs % 60));
        m_table->setItem(i, 4, dur_item);
    }

    m_table->setUpdatesEnabled(true);
}

void PlaylistView::handle_favorite_changed(const QUuid& id, bool is_favorite) {
    if (m_playlist_name == "Favourites") {
        refresh();
        return;
    }

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