#pragma once
#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include "playlist_view.hpp"
#include "track_model.hpp"
#include "player/db.hpp"

namespace ui {

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(player::Db* db, QWidget* parent = nullptr);
    ~LibraryView() override = default;

    void refresh();
    void toggle_search();
    std::vector<player::Track> current_queue() const;

signals:
    void play_requested(const std::vector<player::Track>& queue, size_t index);
    void queue_updated(const std::vector<player::Track>& queue);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setup_ui();
    void handle_favorite_changed(const QUuid& id, bool is_favorite);

    player::Db* m_db{nullptr};
    QTableView* m_table{nullptr};
    TrackModel* m_model{nullptr};
    QLineEdit* m_search_bar{nullptr};
    QLabel* m_header_label{nullptr};
    QLabel* m_sub_label{nullptr};
    TrackDelegate* m_delegate{nullptr};
};

}
