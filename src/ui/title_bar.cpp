#include "title_bar.hpp"
#include <QMouseEvent>
#include <QCoreApplication>
#include "icons.hpp"

namespace ui {

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(48);
    setStyleSheet("background-color: #0c0c0c; border-bottom: 1px solid #141414;");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 0, 0);
    layout->setSpacing(0);

    auto* title_lbl = new QLabel(QString("maya-%1").arg(VERSION), this);
    title_lbl->setStyleSheet("font-size: 13px; font-family: 'Segoe UI', sans-serif; font-weight: 600; color: #3c3c3c; letter-spacing: 1px;");
    layout->addWidget(title_lbl);

    layout->addStretch();

    auto* min_btn = new QPushButton(this);
    min_btn->setObjectName("min_btn");
    min_btn->setFixedSize(46, 48);
    min_btn->setFocusPolicy(Qt::NoFocus);
    min_btn->setCursor(Qt::PointingHandCursor);
    min_btn->setIcon(icons::from_svg(icons::minimize, QColor("#8c8c8c")));
    min_btn->setStyleSheet(R"(
        QPushButton#min_btn { background: transparent; border: none; }
        QPushButton#min_btn:hover { background-color: rgba(255, 255, 255, 15); }
    )");
    connect(min_btn, &QPushButton::clicked, parent, [this]() {
        window()->showMinimized();
    });

    auto* close_btn = new QPushButton(this);
    close_btn->setObjectName("close_btn");
    close_btn->setFixedSize(46, 48);
    close_btn->setFocusPolicy(Qt::NoFocus);
    close_btn->setCursor(Qt::PointingHandCursor);
    close_btn->setIcon(icons::from_svg(icons::close, QColor("#8c8c8c")));
    close_btn->setStyleSheet(R"(
        QPushButton#close_btn { background: transparent; border: none; }
        QPushButton#close_btn:hover { background-color: #e81123; }
    )");
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
