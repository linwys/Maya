#include "equalizer_visualizer.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <bass.h>
#include <cmath>

namespace ui {

EqualizerVisualizer::EqualizerVisualizer(QWidget* parent) 
    : QWidget(parent), m_bars(20, 0.0f), m_peaks(20, 0.0f) {
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        update();
    });
}

void EqualizerVisualizer::start(unsigned long bass_stream_handle) {
    m_bass_stream = bass_stream_handle;
    m_timer.start();
}

void EqualizerVisualizer::stop() {
    m_timer.stop();
    m_bass_stream = 0;
    std::fill(m_bars.begin(), m_bars.end(), 0.0f);
    std::fill(m_peaks.begin(), m_peaks.end(), 0.0f);
    update();
}

void EqualizerVisualizer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    float fft[1024];
    std::fill(std::begin(fft), std::end(fft), 0.0f);

    if (m_bass_stream && BASS_ChannelIsActive(m_bass_stream) == BASS_ACTIVE_PLAYING) {
        BASS_ChannelGetData(m_bass_stream, fft, BASS_DATA_FFT2048);
    }

    const size_t num_bars = m_bars.size();
    const float w = static_cast<float>(width()) / num_bars;
    const float h = static_cast<float>(height());

    for (size_t i = 0; i < num_bars; ++i) {
        size_t b0 = static_cast<size_t>(pow(2.0, i * 10.0 / (num_bars - 1)) * 1024.0 / 1024.0);
        size_t b1 = static_cast<size_t>(pow(2.0, (i + 1) * 10.0 / (num_bars - 1)) * 1024.0 / 1024.0);
        if (b1 <= b0) b1 = b0 + 1;

        float peak = 0.0f;
        for (size_t j = b0; j < b1 && j < 1024; ++j) {
            if (fft[j] > peak) peak = fft[j];
        }

        float val = sqrt(peak) * 3.0f;
        if (val > 1.0f) val = 1.0f;

        m_bars[i] = m_bars[i] * 0.75f + val * 0.25f;
        m_peaks[i] = std::max(m_peaks[i] - 0.01f, m_bars[i]);

        float bar_h = m_bars[i] * h;
        float peak_y = h - (m_peaks[i] * h);

        QRectF rect(i * w + 2, h - bar_h, w - 4, bar_h);
        painter.fillRect(rect, QColor(255, 255, 255, 180));

        painter.setPen(QColor(255, 255, 255, 220));
        painter.drawLine(QPointF(i * w + 2, peak_y), QPointF(i * w + w - 2, peak_y));
    }
}

}