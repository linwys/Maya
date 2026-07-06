#include "search_overlay.hpp"
#include <QKeyEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace ui {

SearchOverlay::SearchOverlay(QWidget* parent) : QWidget(parent) {
    setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignCenter);

    m_label = new QLabel("Searching...", this);
    m_label->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff; background: transparent;");
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);

    m_cancel_btn = new QPushButton("Cancel", this);
    m_cancel_btn->setFixedSize(120, 36);
    m_cancel_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 18px; font-weight: bold; font-size: 13px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(m_cancel_btn, &QPushButton::clicked, this, &SearchOverlay::cancel_requested);
    layout->addWidget(m_cancel_btn);
}

void SearchOverlay::show_overlay(const QString& text) {
    m_label->setText(text);
    setGeometry(parentWidget()->rect());
    setVisible(true);
    raise();
    setFocus();
    grabKeyboard();
}

void SearchOverlay::hide_overlay() {
    releaseKeyboard();
    setVisible(false);
}

void SearchOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit cancel_requested();
    } else {
        event->accept();
    }
}

void SearchOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(12, 12, 12, 230));
}

}