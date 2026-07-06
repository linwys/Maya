#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QButtonGroup>

namespace ui {

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);
    ~Sidebar() override = default;

signals:
    void navigation_changed(int index);

private:
    void create_button(const QString& text, int id);

    QVBoxLayout* m_layout{nullptr};
    QButtonGroup* m_group{nullptr};
};

}