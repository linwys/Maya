#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "player/db.hpp"

namespace ui {

class DragSelectionListWidget : public QListWidget {
    Q_OBJECT
public:
    using QListWidget::QListWidget;

protected:
    void mousePressEvent(QMouseEvent* event) override;      // + 
    void mouseMoveEvent(QMouseEvent* event) override; // + 
    void keyPressEvent(QKeyEvent* event) override; // + 

private:
    bool m_drag_checking{true};
    QListWidgetItem* m_last_drag_item{nullptr};
};

class PlaylistDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlaylistDialog(player::Db* db, QWidget* parent = nullptr);
    ~PlaylistDialog() override = default;

    QString playlist_name() const;
    QString cover_path() const { return m_cover_path; }
    std::vector<QUuid> selected_tracks() const;
    void set_playlist_data(const QString& name, const QString& cover, const std::vector<QUuid>& track_ids);

private:
    player::Db* m_db{nullptr};
    QLineEdit* m_name_input{nullptr};
    QLabel* m_cover_label{nullptr};
    DragSelectionListWidget* m_track_list{nullptr};
    QString m_cover_path;
};

}