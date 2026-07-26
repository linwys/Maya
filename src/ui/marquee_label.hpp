#pragma once
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QResizeEvent>

namespace ui {
class MarqueeLabel : public QLabel {
    Q_OBJECT
public:
    explicit MarqueeLabel(QWidget* parent = nullptr) : QLabel(parent) {
        connect(&m_timer, &QTimer::timeout, this, &MarqueeLabel::on_timer_tick);
    }
    explicit MarqueeLabel(const QString& text, QWidget* parent = nullptr) : QLabel(text, parent) {
        connect(&m_timer, &QTimer::timeout, this, &MarqueeLabel::on_timer_tick);
        update_scrolling_state();
    }

    void setText(const QString& text) {
        QLabel::setText(text);
        update_scrolling_state();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (!m_scrolling) {
            QLabel::paintEvent(event);
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QString txt = text();
        int tw = fontMetrics().horizontalAdvance(txt);
        int gap = 50; 
        QRect r = contentsRect();
        int y = r.y() + (r.height() - fontMetrics().height()) / 2 + fontMetrics().ascent();
        painter.setClipRect(r);
        painter.setPen(palette().color(foregroundRole()));
        painter.drawText(r.x() - m_scroll_pos, y, txt);
        painter.drawText(r.x() - m_scroll_pos + tw + gap, y, txt);
    }

    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        update_scrolling_state();
    }

private:
    void update_scrolling_state() {
        int tw = fontMetrics().horizontalAdvance(text());
        int available_w = contentsRect().width();
        if (tw > available_w && !text().isEmpty()) {
            if (!m_scrolling) {
                m_scrolling = true;
                m_scroll_pos = 0;
                m_delay_ticks = 50; //pause on start
                m_timer.start(30);
            }
        } else {
            if (m_scrolling) {
                m_scrolling = false;
                m_timer.stop();
                m_scroll_pos = 0;
                update();
            }
        }
    }

    void on_timer_tick() {
        int tw = fontMetrics().horizontalAdvance(text());
        int gap = 50;
        
        if (m_delay_ticks > 0) {
            m_delay_ticks--;
            return;
        }

        m_scroll_pos++;
        if (m_scroll_pos >= tw + gap) {
            m_scroll_pos = 0;
            m_delay_ticks = 50;//pause at stop
        }
        update();
    }

    QTimer m_timer;
    bool m_scrolling{false};
    int m_scroll_pos{0};
    int m_delay_ticks{0};
};//MarqueeLabel

}//ui

