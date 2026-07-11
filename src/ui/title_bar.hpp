#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPoint>

namespace ui {

class TitleBar : public QWidget {
    Q_OBJECT
public:
    static inline const QString VERSION = "1.1.0-beta";

    explicit TitleBar(QWidget* parent = nullptr);
    ~TitleBar() override = default;

signals:
    void settings_clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint m_drag_position;
    QPushButton* m_settings_btn{nullptr};
};

}
