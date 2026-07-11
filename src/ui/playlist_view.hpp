#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableView>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <vector>
#include "track_model.hpp"
#include "player/db.hpp"

namespace ui {

class TrackDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

class PlaylistHeader : public QWidget {
    Q_OBJECT
public:
    using QWidget::QWidget;
    
    void set_data(const QPixmap& cover, const QColor& theme_bg);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_cover;
    QColor m_theme_bg{12, 12, 12};
};

class PlaylistView : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistView(player::Db* db, QWidget* parent = nullptr);
    ~PlaylistView() override = default;

    void open_playlist(const QString& name);
    void set_theme_colors(const QColor& bg_color);
    QString current_playlist_name() const { return m_playlist_name; }
    void toggle_search();
    std::vector<player::Track> current_queue() const;

signals:
    void play_requested(const std::vector<player::Track>& queue, size_t index);
    void queue_updated(const std::vector<player::Track>& queue);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setup_ui();
    void refresh();
    void handle_favorite_changed(const QUuid& id, bool is_favorite);

    player::Db* m_db{nullptr};
    QString m_playlist_name;
    std::vector<player::Track> m_tracks;
    QPixmap m_current_cover;
    QColor m_theme_bg{12, 12, 12};

    PlaylistHeader* m_header_widget{nullptr};
    QLabel* m_cover_label{nullptr};
    QLabel* m_title_label{nullptr};
    QLabel* m_sub_label{nullptr};
    QTableView* m_table{nullptr};
    TrackModel* m_model{nullptr};
    QLineEdit* m_search_bar{nullptr};
    TrackDelegate* m_delegate{nullptr};
};

}
