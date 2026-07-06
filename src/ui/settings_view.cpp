#include "settings_view.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QScrollArea>

namespace ui {

ToggleSwitch::ToggleSwitch(QWidget* parent) : QWidget(parent) {
    setFixedSize(44, 24);
}

void ToggleSwitch::set_checked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        update();
        emit toggled(m_checked);
    }
}

void ToggleSwitch::set_colors(const QColor& active_bg, const QColor& inactive_bg, const QColor& thumb_color) {
    m_active_bg = active_bg;
    m_inactive_bg = inactive_bg;
    m_thumb_color = thumb_color;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bg_color;
    if (!isEnabled()) {
        bg_color = QColor("#1e1e1e");
    } else {
        bg_color = m_checked ? m_active_bg : m_inactive_bg;
    }
    painter.setBrush(bg_color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 12, 12);

    QColor thumb_color;
    if (!isEnabled()) {
        thumb_color = QColor("#444444");
    } else {
        thumb_color = m_thumb_color;
    }
    painter.setBrush(thumb_color);
    int x = m_checked ? 24 : 4;
    painter.drawEllipse(x, 4, 16, 16);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent* event) {
    if (!isEnabled()) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        set_checked(!m_checked);
    }
}

SegmentedControl::SegmentedControl(const QStringList& segments, QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    for (const auto& text : segments) {
        auto* btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setFixedSize(80, 28);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #1a1a1a;
                color: #8c8c8c;
                border: 1px solid #333333;
                border-radius: 4px;
                font-size: 12px;
                font-weight: bold;
            }
            QPushButton:checked {
                background-color: #ffffff;
                color: #000000;
                border: none;
            }
            QPushButton:disabled {
                background-color: #121212;
                border: 1px solid #1a1a1a;
                color: #444444;
            }
        )");

        connect(btn, &QPushButton::clicked, this, [this, text]() {
            set_current(text);
            emit changed(m_current);
        });

        layout->addWidget(btn);
        m_buttons.push_back(btn);
    }

    if (!segments.isEmpty()) {
        set_current(segments[0]);
    }
}

void SegmentedControl::set_current(const QString& val) {
    m_current = val;
    for (auto* btn : m_buttons) {
        btn->setChecked(btn->text().toLower() == val.toLower());
    }
}

SettingsView::SettingsView(QWidget* parent) : QWidget(parent) {
    setup_ui();
}

void SettingsView::setup_ui() {
    auto* outer_layout = new QVBoxLayout(this);
    outer_layout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("background-color: transparent; border: none;");

    auto* container = new QWidget(scroll);
    container->setObjectName("settings_container");
    m_main_layout = new QVBoxLayout(container);
    m_main_layout->setContentsMargins(32, 32, 32, 32);
    m_main_layout->setSpacing(24);

    auto* header_label = new QLabel("Settings", container);
    header_label->setStyleSheet("font-size: 28px; font-weight: bold;");
    m_main_layout->addWidget(header_label);

    auto* row1 = create_toggle_row("Crossfade", "Blend the end of one track into the next", m_crossfade_sw);
    auto* row2 = create_toggle_row("Gapless playback", "No silence between consecutive tracks", m_gapless_sw);
    auto* row3 = create_toggle_row("Normalize volume", "Keep loudness consistent across tracks", m_normalize_sw);
    
    m_main_layout->addWidget(create_card("Playback", {row1, row2, row3}));

    connect(m_normalize_sw, &ToggleSwitch::toggled, this, &SettingsView::normalize_toggled);

    auto* row4 = create_toggle_row("Auto-download favorites", "Save liked online tracks for offline play", m_auto_download_sw);
    auto* row5 = create_toggle_row("Wi-Fi only", "Pause transfers on metered connections", m_wifi_only_sw);
    
    m_format_control = new SegmentedControl({"MP3", "FLAC", "OPUS"}, this);
    m_format_control->setEnabled(true);
    auto* row_format = create_control_row("Save format", m_format_control);

    m_main_layout->addWidget(create_card("Downloads", {row4, row5, row_format}));

    m_theme_control = new SegmentedControl({"Paper", "Sepia", "Auto"}, this);
    m_theme_control->setEnabled(false);
    m_theme_control->setToolTip("temporary unavailable");
    auto* row_theme = create_control_row("Theme", m_theme_control);

    m_config_dir_btn = new QPushButton("Select Folder", this);
    m_config_dir_btn->setFixedHeight(32);
    m_config_dir_btn->setFocusPolicy(Qt::NoFocus);
    m_config_dir_btn->setStyleSheet(R"(
        QPushButton {
            background-color: #1a1a1a; color: #ffffff; border-radius: 6px; font-weight: bold; font-size: 12px; border: 1px solid #333333;
        }
        QPushButton:hover { background-color: #262626; }
    )");
    connect(m_config_dir_btn, &QPushButton::clicked, this, &SettingsView::config_dir_requested);
    auto* row_config = create_control_row("Config folder", m_config_dir_btn);

    auto* row6 = create_toggle_row("Animated equalizer", "Show dancing bars on the playing track", m_eq_sw);

    auto* row_discord = create_toggle_row("Discord Rich Presence", "Display current track on Discord profile", m_discord_rpc_sw);

    m_main_layout->addWidget(create_card("Appearance", {row6, row_theme, row_config, row_discord}));

    connect(m_eq_sw, &ToggleSwitch::toggled, this, &SettingsView::equalizer_toggled);
    connect(m_discord_rpc_sw, &ToggleSwitch::toggled, this, &SettingsView::discord_rpc_toggled);

    m_main_layout->addStretch();
    scroll->setWidget(container);
    outer_layout->addWidget(scroll);
}

QFrame* SettingsView::create_card(const QString& title, const std::vector<QWidget*>& items) {
    auto* card = new QFrame(this);
    card->setObjectName("settings_card");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    auto* title_lbl = new QLabel(title, card);
    title_lbl->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(title_lbl);

    for (auto* item : items) {
        layout->addWidget(item);
    }

    return card;
}

QWidget* SettingsView::create_toggle_row(const QString& title, const QString& desc, ToggleSwitch*& out_toggle) {
    auto* row = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* text_layout = new QVBoxLayout();
    text_layout->setSpacing(2);
    auto* title_lbl = new QLabel(title, row);
    title_lbl->setStyleSheet("font-size: 14px; font-weight: 500;");
    auto* desc_lbl = new QLabel(desc, row);
    desc_lbl->setStyleSheet("font-size: 12px; color: #8c8c8c;");
    text_layout->addWidget(title_lbl);
    text_layout->addWidget(desc_lbl);

    layout->addLayout(text_layout, 1);

    out_toggle = new ToggleSwitch(row);
    layout->addWidget(out_toggle);

    return row;
}

QWidget* SettingsView::create_control_row(const QString& title, QWidget* control) {
    auto* row = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* title_lbl = new QLabel(title, row);
    title_lbl->setStyleSheet("font-size: 14px; font-weight: 500;");
    layout->addWidget(title_lbl, 1);

    layout->addWidget(control);
    return row;
}

void SettingsView::update_config_dir_label(const QString& dir_path) {
    QString abbreviated = dir_path;
    if (abbreviated.length() > 25) {
        abbreviated = "..." + abbreviated.right(22);
    }
    m_config_dir_btn->setText(abbreviated);
}

void SettingsView::initialize_settings(
    bool crossfade,
    bool gapless,
    bool normalize,
    bool auto_dl,
    bool wifi,
    bool eq,
    bool discord_rpc,
    const QString& format,
    const QString& theme,
    const QString& config_dir
) {
    m_crossfade_sw->set_checked(crossfade);
    m_gapless_sw->set_checked(gapless);
    m_normalize_sw->set_checked(normalize);
    m_auto_download_sw->set_checked(auto_dl);
    m_wifi_only_sw->set_checked(wifi);
    m_eq_sw->set_checked(eq);
    m_discord_rpc_sw->set_checked(discord_rpc);
    m_format_control->set_current(format);
    m_theme_control->set_current(theme);
    update_config_dir_label(config_dir);
}

void SettingsView::initialize_format_selection(const QString& current_format, std::function<void(const QString&)> callback) {
    m_format_control->set_current(current_format);
    connect(m_format_control, &SegmentedControl::changed, this, [callback](const QString& format) {
        callback(format.toLower());
    });
}

void SettingsView::initialize_theme_selection(const QString& current_theme, std::function<void(const QString&)> callback) {
    m_theme_control->set_current(current_theme);
    connect(m_theme_control, &SegmentedControl::changed, this, [callback](const QString& theme) {
        callback(theme.toLower());
    });
}

}