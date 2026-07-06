#include <QApplication>
#include <QProxyStyle>
#include "ui/main_window.hpp"
#include "player/track.hpp"

class NoFocusProxyStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override {
        if (element == PE_FrameFocusRect) {
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle(new NoFocusProxyStyle());

    qRegisterMetaType<player::Track>();

    QPalette dark_palette;
    dark_palette.setColor(QPalette::Window, QColor("#0c0c0c"));
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Base, QColor("#121212"));
    dark_palette.setColor(QPalette::AlternateBase, QColor("#0c0c0c"));
    dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::Button, QColor("#0c0c0c"));
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::red);
    dark_palette.setColor(QPalette::Link, QColor("#4285f4"));
    dark_palette.setColor(QPalette::Highlight, QColor("#2a2a2a"));
    dark_palette.setColor(QPalette::HighlightedText, Qt::white);

    app.setPalette(dark_palette);

    ui::MainWindow window;
    window.show();

    return app.exec();
}