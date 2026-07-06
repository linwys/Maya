#include "sidebar.hpp"
#include "icons.hpp"

namespace ui {

Sidebar::Sidebar(QWidget* parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(16, 24, 16, 24);
    m_layout->setSpacing(8);

    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);

    auto color_normal = QColor("#8c8c8c");

    auto* btn0 = new QPushButton("Library", this);
    btn0->setIcon(icons::from_svg(icons::library, color_normal));
    btn0->setCheckable(true);
    btn0->setChecked(true);
    btn0->setFocusPolicy(Qt::NoFocus);
    m_layout->addWidget(btn0);
    m_group->addButton(btn0, 0);

    auto* btn1 = new QPushButton("Playlists", this);
    btn1->setIcon(icons::from_svg(icons::playlists, color_normal));
    btn1->setCheckable(true);
    btn1->setFocusPolicy(Qt::NoFocus);
    m_layout->addWidget(btn1);
    m_group->addButton(btn1, 1);

    auto* btn2 = new QPushButton("Discover", this);
    btn2->setIcon(icons::from_svg(icons::discover, color_normal));
    btn2->setCheckable(true);
    btn2->setFocusPolicy(Qt::NoFocus);
    m_layout->addWidget(btn2);
    m_group->addButton(btn2, 2);

    auto* btn3 = new QPushButton("Downloads", this);
    btn3->setIcon(icons::from_svg(icons::downloads, color_normal));
    btn3->setCheckable(true);
    btn3->setFocusPolicy(Qt::NoFocus);
    m_layout->addWidget(btn3);
    m_group->addButton(btn3, 3);

    auto* btn4 = new QPushButton("Settings", this);
    btn4->setIcon(icons::from_svg(icons::settings, color_normal));
    btn4->setCheckable(true);
    btn4->setFocusPolicy(Qt::NoFocus);
    m_layout->addWidget(btn4);
    m_group->addButton(btn4, 4);

    m_layout->addStretch();

    setStyleSheet(R"(
        QWidget {
            background-color: #0c0c0c;
        }
        QPushButton {
            background: none;
            border: none;
            color: #8c8c8c;
            padding: 10px 16px;
            font-size: 14px;
            font-weight: 500;
            text-align: left;
            border-radius: 6px;
            outline: none;
        }
        QPushButton:hover {
            background-color: #1a1a1a;
            color: #ffffff;
        }
        QPushButton:checked {
            background-color: #1e1e1e;
            color: #ffffff;
        }
    )");

    connect(m_group, &QButtonGroup::idClicked, this, &Sidebar::navigation_changed);
}

}