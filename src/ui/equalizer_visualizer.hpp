#pragma once
#include <QWidget>
#include <QTimer>
#include <vector>

namespace ui {

class EqualizerVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit EqualizerVisualizer(QWidget* parent = nullptr);
    ~EqualizerVisualizer() override = default;

    void start(unsigned long bass_stream_handle);
    void stop();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    unsigned long m_bass_stream{0};
    QTimer m_timer;
    std::vector<float> m_bars;
    std::vector<float> m_peaks;
};

}