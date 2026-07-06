#include "playlist_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QKeyEvent>

namespace ui {
void DragSelectionListWidget::mousePressEvent(QMouseEvent* event) {
    if (auto* item = itemAt(event->pos())) {
        setCurrentItem(item);
        m_drag_checking = (item->checkState() == Qt::Unchecked);
        item->setCheckState(m_drag_checking ? Qt::Checked : Qt::Unchecked);
        m_last_drag_item = item;
        event->accept();
    } else {
        QListWidget::mousePressEvent(event);
    }
}

void DragSelectionListWidget::mouseMoveEvent(QMouseEvent* event) {
    if (auto* item = itemAt(event->pos())) {
        if (item != m_last_drag_item) {
            item->setCheckState(m_drag_checking ? Qt::Checked : Qt::Unchecked);
            m_last_drag_item = item;
        }
    }
    event->accept();
}

void DragSelectionListWidget::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A) {
        bool any_unchecked = false;
        for (int i = 0; i < count(); ++i) {
            if (item(i)->checkState() == Qt::Unchecked) {
                any_unchecked = true;
                break;
            }
        }
        for (int i = 0; i < count(); ++i) {
            item(i)->setCheckState(any_unchecked ? Qt::Checked : Qt::Unchecked);
        }
        event->accept();
        return;
    }
    QListWidget::keyPressEvent(event);
}

PlaylistDialog::PlaylistDialog(player::Db* db, QWidget* parent) 
    : QDialog(parent), m_db(db) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(480, 520);
    setStyleSheet("background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px;");

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(24, 24, 24, 24);
    main_layout->setSpacing(16);

    auto* header = new QLabel("Create Playlist", this);
    header->setStyleSheet("font-size: 20px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    main_layout->addWidget(header);

    auto* top_section = new QHBoxLayout();
    m_cover_label = new QLabel(this);
    m_cover_label->setFixedSize(100, 100);
    m_cover_label->setStyleSheet("background-color: #1a1a1a; border-radius: 8px; border: 1px dashed #333333;");
    m_cover_label->setAlignment(Qt::AlignCenter);
    m_cover_label->setText("No Cover");
    top_section->addWidget(m_cover_label);

    auto* top_right = new QVBoxLayout();
    m_name_input = new QLineEdit(this);
    m_name_input->setPlaceholderText("Playlist Name");
    m_name_input->setStyleSheet(R"(
        QLineEdit {
            background-color: #1a1a1a; border: 1px solid #333333; border-radius: 6px; padding: 8px 12px; color: #ffffff; font-size: 14px;
        }
    )");
    top_right->addWidget(m_name_input);

    auto* select_cover_btn = new QPushButton("Choose Image", this);
    select_cover_btn->setFixedHeight(32);
    select_cover_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 6px; font-weight: bold; font-size: 12px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(select_cover_btn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Cover", "", "Images (*.png *.jpg *.jpeg)");
        if (!file.isEmpty()) {
            m_cover_path = file;
            QPixmap pix(file);
            m_cover_label->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    });
    top_right->addWidget(select_cover_btn);
    top_section->addLayout(top_right);
    main_layout->addLayout(top_section);

    auto* list_lbl = new QLabel("Add Tracks:", this);
    list_lbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #8c8c8c; border: none; background: transparent;");
    main_layout->addWidget(list_lbl);

    m_track_list = new DragSelectionListWidget(this);
    m_track_list->setStyleSheet(R"(
        QListWidget {
            background-color: #1a1a1a; border: 1px solid #1a1a1a; border-radius: 8px; color: #ffffff; padding: 4px;
        }
        QListWidget::item { padding: 8px; border-bottom: 1px solid #141414; }
        QListWidget::item:selected { background-color: #262626; }
    )");

    for (const auto& track : m_db->tracks()) {
        auto* item = new QListWidgetItem(QString("%1 — %2").arg(track.title).arg(track.artist), m_track_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, track.id.toString());
        m_track_list->addItem(item);
    }

    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(12);

    auto* cancel_btn = new QPushButton("Cancel", this);
    cancel_btn->setFixedHeight(36);
    cancel_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 18px; font-weight: bold; font-size: 13px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

    auto* create_btn = new QPushButton("Save", this);
    create_btn->setFixedHeight(36);
    create_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff; color: #000000; border-radius: 18px; font-weight: bold; font-size: 13px; border: none;
        }
        QPushButton:hover { background-color: #e6e6e6; }
    )");
    connect(create_btn, &QPushButton::clicked, this, &QDialog::accept);

    main_layout->addWidget(m_track_list);
    btn_layout->addWidget(cancel_btn, 1);
    btn_layout->addWidget(create_btn, 1);
    main_layout->addLayout(btn_layout);
}

QString PlaylistDialog::playlist_name() const {
    return m_name_input->text().trimmed();
}

std::vector<QUuid> PlaylistDialog::selected_tracks() const {
    std::vector<QUuid> ids;
    for (int i = 0; i < m_track_list->count(); ++i) {
        auto* item = m_track_list->item(i);
        if (item->checkState() == Qt::Checked) {
            ids.push_back(QUuid::fromString(item->data(Qt::UserRole).toString()));
        }
    }
    return ids;
}

void PlaylistDialog::set_playlist_data(const QString& name, const QString& cover, const std::vector<QUuid>& track_ids) {
    m_name_input->setText(name);
    m_name_input->setEnabled(false);
    if (!cover.isEmpty() && QFile::exists(cover)) {
        m_cover_path = cover;
        QPixmap pix(cover);
        m_cover_label->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    
    for (int i = 0; i < m_track_list->count(); ++i) {
        auto* item = m_track_list->item(i);
        QUuid id = QUuid::fromString(item->data(Qt::UserRole).toString());
        if (std::find(track_ids.begin(), track_ids.end(), id) != track_ids.end()) {
            item->setCheckState(Qt::Checked);
        }
    }
}

}