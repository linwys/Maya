#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "player/db.hpp"

namespace ui {

class PlaylistsView : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistsView(player::Db* db, QWidget* parent = nullptr);
    ~PlaylistsView() override = default;

    void refresh();

private:
    void setup_ui();

    player::Db* m_db{nullptr};
    QGridLayout* m_grid_layout{nullptr};
};

}