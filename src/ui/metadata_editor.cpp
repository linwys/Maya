#include "metadata_editor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

namespace ui {

MetadataEditorDialog::MetadataEditorDialog(const player::Track& track, QWidget* parent) : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 380);
    setStyleSheet("background-color: #121212; border: 1px solid #1a1a1a; border-radius: 12px;");

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(24, 24, 24, 24);
    main_layout->setSpacing(14);

    auto* header = new QLabel("Edit Track Info", this);
    header->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    main_layout->addWidget(header);

    auto* mid_section = new QHBoxLayout();
    
    m_cover_label = new QLabel(this);
    m_cover_label->setFixedSize(80, 80);
    m_cover_label->setStyleSheet("background-color: #1a1a1a; border-radius: 6px; border: 1px dashed #333333;");
    m_cover_label->setAlignment(Qt::AlignCenter);
    m_cover_label->setText("No Cover");
    mid_section->addWidget(m_cover_label);

    auto* cover_btn_layout = new QVBoxLayout();
    auto* choose_cover_btn = new QPushButton("Choose Cover", this);
    choose_cover_btn->setFixedHeight(32);
    choose_cover_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 6px; font-weight: bold; font-size: 11px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(choose_cover_btn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Artwork", "", "Images (*.png *.jpg *.jpeg)");
        if (!file.isEmpty()) {
            m_cover_path = file;
            QPixmap pix(file);
            m_cover_label->setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    });
    cover_btn_layout->addWidget(choose_cover_btn);
    cover_btn_layout->addStretch();
    mid_section->addLayout(cover_btn_layout);
    main_layout->addLayout(mid_section);

    m_title_input = new QLineEdit(this);
    m_title_input->setPlaceholderText("Title");
    m_title_input->setText(track.title);
    
    m_artist_input = new QLineEdit(this);
    m_artist_input->setPlaceholderText("Artist");
    m_artist_input->setText(track.artist);

    m_album_input = new QLineEdit(this);
    m_album_input->setPlaceholderText("Album");
    m_album_input->setText(track.album);

    QString style = R"(
        QLineEdit {
            background-color: #1a1a1a; border: 1px solid #333333; border-radius: 6px; padding: 8px 12px; color: #ffffff; font-size: 13px;
        }
    )";
    m_title_input->setStyleSheet(style);
    m_artist_input->setStyleSheet(style);
    m_album_input->setStyleSheet(style);

    main_layout->addWidget(m_title_input);
    main_layout->addWidget(m_artist_input);
    main_layout->addWidget(m_album_input);

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

    auto* save_btn = new QPushButton("Save", this);
    save_btn->setFixedHeight(36);
    save_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff; color: #000000; border-radius: 18px; font-weight: bold; font-size: 13px; border: none;
        }
        QPushButton:hover { background-color: #e6e6e6; }
    )");
    connect(save_btn, &QPushButton::clicked, this, &QDialog::accept);

    btn_layout->addWidget(cancel_btn, 1);
    btn_layout->addWidget(save_btn, 1);
    main_layout->addLayout(btn_layout);
}

QString MetadataEditorDialog::title() const {
    return m_title_input->text().trimmed();
}

QString MetadataEditorDialog::artist() const {
    return m_artist_input->text().trimmed();
}

QString MetadataEditorDialog::album() const {
    return m_album_input->text().trimmed();
}

}