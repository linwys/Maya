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
#include <QKeyEvent>
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
        if (m_playlist_name == "Favourites") refresh();
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

    m_search_bar = new QLineEdit(this);
    m_search_bar->setPlaceholderText("Search inside playlist... (Ctrl + F to hide)");
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
        auto* remove_act = menu->addAction("Remove from Playlist");

        QAction* selected = menu->exec(m_table->viewport()->mapToGlobal(pos));
        if (selected == play_act) {
            emit play_requested(current_queue(), static_cast<size_t>(row));
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
    m_model->set_tracks(&m_tracks);
    m_table->setUpdatesEnabled(true);
    emit queue_updated(current_queue());
}

void PlaylistView::handle_favorite_changed(const QUuid&, bool) {
    refresh();
}

void PlaylistView::toggle_search() {
    m_search_bar->setVisible(!m_search_bar->isVisible());
    if (m_search_bar->isVisible()) {
        m_search_bar->setFocus();
    } else {
        m_search_bar->clear();
    }
}

std::vector<player::Track> PlaylistView::current_queue() const {
    std::vector<player::Track> queue;
    queue.reserve(m_model->rowCount());
    for (int i = 0; i < m_model->rowCount(); ++i) {
        queue.push_back(m_model->get_track(i));
    }
    return queue;
}

void PlaylistView::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_F) {
        toggle_search();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

}
