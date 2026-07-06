#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <vector>
#include <functional>

namespace ui {

class ToggleSwitch : public QWidget {
    Q_OBJECT
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    ~ToggleSwitch() override = default;

    bool is_checked() const { return m_checked; }
    void set_checked(bool checked);
    void set_colors(const QColor& active_bg, const QColor& inactive_bg, const QColor& thumb_color);

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool m_checked{false};
    QColor m_active_bg{"#ffffff"};
    QColor m_inactive_bg{"#262626"};
    QColor m_thumb_color{"#000000"};
};

class SegmentedControl : public QWidget {
    Q_OBJECT
public:
    explicit SegmentedControl(const QStringList& segments, QWidget* parent = nullptr);
    ~SegmentedControl() override = default;

    QString current_segment() const { return m_current; }
    void set_current(const QString& val);

signals:
    void changed(const QString& val);

private:
    QString m_current;
    std::vector<QPushButton*> m_buttons;
};

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(QWidget* parent = nullptr);
    ~SettingsView() override = default;

    void initialize_format_selection(const QString& current_format, std::function<void(const QString&)> callback);
    void initialize_theme_selection(const QString& current_theme, std::function<void(const QString&)> callback);
    void initialize_settings(
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
    );
    void update_config_dir_label(const QString& dir_path);

signals:
    void crossfade_toggled(bool checked);
    void gapless_toggled(bool checked);
    void normalize_toggled(bool checked);
    void auto_download_toggled(bool checked);
    void wifi_only_toggled(bool checked);
    void equalizer_toggled(bool checked);
    void discord_rpc_toggled(bool checked);
    void config_dir_requested();

private:
    void setup_ui();
    QFrame* create_card(const QString& title, const std::vector<QWidget*>& items);
    QWidget* create_toggle_row(const QString& title, const QString& desc, ToggleSwitch*& out_toggle);
    QWidget* create_control_row(const QString& title, QWidget* control);

    QVBoxLayout* m_main_layout{nullptr};
    SegmentedControl* m_format_control{nullptr};
    SegmentedControl* m_theme_control{nullptr};
    
    ToggleSwitch* m_crossfade_sw{nullptr};
    ToggleSwitch* m_gapless_sw{nullptr};
    ToggleSwitch* m_normalize_sw{nullptr};
    ToggleSwitch* m_auto_download_sw{nullptr};
    ToggleSwitch* m_wifi_only_sw{nullptr};
    ToggleSwitch* m_eq_sw{nullptr};
    ToggleSwitch* m_discord_rpc_sw{nullptr};
    QPushButton* m_config_dir_btn{nullptr};
};

}