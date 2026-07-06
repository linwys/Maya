#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace ui {

class SearchOverlay : public QWidget {
    Q_OBJECT
public:
    explicit SearchOverlay(QWidget* parent = nullptr);
    ~SearchOverlay() override = default;

    void show_overlay(const QString& text);
    void hide_overlay();

signals:
    void cancel_requested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_label{nullptr};
    QPushButton* m_cancel_btn{nullptr};
};

}