#include "title_bar.hpp"
#include <QMouseEvent>
#include <QCoreApplication>

namespace ui {

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(48);
    setStyleSheet("background-color: #0c0c0c; border-bottom: 1px solid #141414;");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(12);

    auto* title_lbl = new QLabel("", this);
    title_lbl->setStyleSheet("font-size: 28px; font-weight: bold; color: #ffffff; letter-spacing: 1px;");
    layout->addWidget(title_lbl);

    layout->addStretch();

    auto* min_btn = new QPushButton(this);
    min_btn->setFixedSize(14, 14);
    min_btn->setFocusPolicy(Qt::NoFocus);
    min_btn->setStyleSheet("background-color: #ffbd2e; border: none; border-radius: 7px;");
    connect(min_btn, &QPushButton::clicked, parent, [this]() {
        window()->showMinimized();
    });

    auto* close_btn = new QPushButton(this);
    close_btn->setFixedSize(14, 14);
    close_btn->setFocusPolicy(Qt::NoFocus);
    close_btn->setStyleSheet("background-color: #ff5f56; border: none; border-radius: 7px;");
    connect(close_btn, &QPushButton::clicked, qApp, &QCoreApplication::quit);

    layout->addWidget(min_btn);
    layout->addWidget(close_btn);
}

void TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_drag_position = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        window()->move(event->globalPosition().toPoint() - m_drag_position);
        event->accept();
    }
}

}
