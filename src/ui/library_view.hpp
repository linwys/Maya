#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "playlist_view.hpp"
#include "player/db.hpp"

namespace ui {

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(player::Db* db, QWidget* parent = nullptr);
    ~LibraryView() override = default;

    void refresh();

signals:
    void play_requested(const std::vector<player::Track>& queue, size_t index);

private:
    void setup_ui();
    void handle_favorite_changed(const QUuid& id, bool is_favorite);

    player::Db* m_db{nullptr};
    QTableWidget* m_table{nullptr};
    QLabel* m_header_label{nullptr};
    QLabel* m_sub_label{nullptr};
    TrackDelegate* m_delegate{nullptr};
};

}