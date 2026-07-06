#include "audio_pipeline.hpp"
#include <bass.h>
#include <algorithm>
#include <QDir>

namespace player {

AudioPipeline::AudioPipeline(QObject* parent) : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setInterval(200);//write
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_state == PlaybackState::Playing && m_bass_stream) {
            emit position_updated(current_position());
        }
    });
}

AudioPipeline::~AudioPipeline() {
    stop();
    BASS_Free();
}

void AudioPipeline::initialize() {
    BASS_SetConfig(BASS_CONFIG_SRC, 6);
    BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);
    BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, 30);
    BASS_SetConfig(BASS_CONFIG_BUFFER, 500);

    if (!BASS_Init(-1, 0, 0, nullptr, nullptr)) {
        BASS_Init(-1, 44100, 0, nullptr, nullptr);
    }

#if defined(Q_OS_WIN)
    BASS_PluginLoad("bassflac.dll", 0);
#else
    BASS_PluginLoad("libbassflac.so", 0);
#endif
}

void AudioPipeline::play(const Track& track) {
    stop();

    m_current_track = track;
    
    QString native_path = QDir::toNativeSeparators(track.file_path);

#if defined(Q_OS_WIN)
    const void* file_ptr = native_path.utf16();
    unsigned long unicode_flag = BASS_UNICODE;
#else
    QByteArray path_utf8 = native_path.toUtf8();
    const void* file_ptr = path_utf8.constData();
    unsigned long unicode_flag = BASS_UNICODE;
#endif

    m_bass_stream = BASS_StreamCreateFile(FALSE, file_ptr, 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN | unicode_flag);
    
    if (!m_bass_stream) {
        return;
    }

    BASS_ChannelSetAttribute(m_bass_stream, BASS_ATTRIB_VOL, 0.0f);
    BASS_ChannelPlay(m_bass_stream, FALSE);
    BASS_ChannelSlideAttribute(m_bass_stream, BASS_ATTRIB_VOL, m_volume, 1000);

    BASS_ChannelSetSync(m_bass_stream, BASS_SYNC_END, 0, &end_sync_proc, this);

    if (m_normalize_enabled) {
        set_normalize(true);
    }

    m_state = PlaybackState::Playing;
    m_timer->start();
    
    emit track_changed(m_current_track);
    emit state_changed(m_state);
}

void AudioPipeline::play_queue(const std::vector<Track>& queue, size_t start_index) {
    m_queue = queue;
    m_current_index = start_index;
    if (start_index < m_queue.size()) {
        play(m_queue[start_index]);
    }
}

void AudioPipeline::pause() {
    if (m_state == PlaybackState::Playing && m_bass_stream) { //fadeout
        BASS_ChannelSlideAttribute(m_bass_stream, BASS_ATTRIB_VOL, 0.0f, 500);
        QTimer::singleShot(500, this, [this]() {
            if (m_bass_stream && m_state == PlaybackState::Playing) {
                BASS_ChannelPause(m_bass_stream);
                m_state = PlaybackState::Paused;
                emit state_changed(m_state);
            }
        });
    }
}

void AudioPipeline::resume() {
    if (m_state == PlaybackState::Paused && m_bass_stream) { //fadein
        BASS_ChannelPlay(m_bass_stream, FALSE);
        BASS_ChannelSlideAttribute(m_bass_stream, BASS_ATTRIB_VOL, m_volume, 1000);
        m_state = PlaybackState::Playing;
        emit state_changed(m_state);
    }
}

void AudioPipeline::stop() {
    m_timer->stop();
    m_fx_normalize = 0;
    if (m_bass_stream) {
        BASS_ChannelStop(m_bass_stream);
        BASS_StreamFree(m_bass_stream);
        m_bass_stream = 0;
    }
    m_state = PlaybackState::Stopped;
    emit state_changed(m_state);
}

void AudioPipeline::seek(int seconds) {
    if (m_bass_stream) {
        BASS_ChannelSlideAttribute(m_bass_stream, BASS_ATTRIB_VOL, 0.0f, 100);
        QTimer::singleShot(100, this, [this, seconds]() {
            if (m_bass_stream) {
                uint64_t pos_bytes = BASS_ChannelSeconds2Bytes(m_bass_stream, static_cast<double>(seconds));
                BASS_ChannelSetPosition(m_bass_stream, pos_bytes, BASS_POS_BYTE);
                BASS_ChannelSlideAttribute(m_bass_stream, BASS_ATTRIB_VOL, m_volume, 1000);
            }
        });
    }
}

void AudioPipeline::set_volume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
    if (m_bass_stream) {
        BASS_ChannelSetAttribute(m_bass_stream, BASS_ATTRIB_VOL, m_volume);
    }
}

void AudioPipeline::set_normalize(bool enabled) {
    m_normalize_enabled = enabled;
    if (m_bass_stream) {
        if (m_normalize_enabled && !m_fx_normalize) {
            m_fx_normalize = BASS_ChannelSetFX(m_bass_stream, BASS_FX_DX8_COMPRESSOR, 0);
            if (m_fx_normalize) {
                BASS_DX8_COMPRESSOR params;
                params.fGain = 0.0f;
                params.fAttack = 10.0f;
                params.fRelease = 200.0f;
                params.fThreshold = -12.0f;
                params.fRatio = 3.0f;
                params.fPredelay = 4.0f;
                BASS_FXSetParameters(m_fx_normalize, &params);
            }
        } else if (!m_normalize_enabled && m_fx_normalize) {
            BASS_ChannelRemoveFX(m_bass_stream, m_fx_normalize);
            m_fx_normalize = 0;
        }
    }
}

void AudioPipeline::next() {
    if (m_queue.empty()) return;
    if (m_repeat == RepeatMode::One) {
        play(m_queue[m_current_index]);
        return;
    }
    if (m_shuffle) {
        m_current_index = static_cast<size_t>(rand() % static_cast<int>(m_queue.size()));
    } else {
        m_current_index = (m_current_index + 1) % m_queue.size();
        if (m_current_index == 0 && m_repeat == RepeatMode::Off) {
            stop();
            return;
        }
    }
    play(m_queue[m_current_index]);
}

void AudioPipeline::prev() {
    if (m_queue.empty()) return;
    if (current_position().count() > 3) {
        seek(0);
        return;
    }
    if (m_shuffle) {
        m_current_index = static_cast<size_t>(rand() % static_cast<int>(m_queue.size()));
    } else {
        if (m_current_index == 0) {
            m_current_index = m_queue.size() - 1;
        } else {
            m_current_index--;
        }
    }
    play(m_queue[m_current_index]);
}

void AudioPipeline::set_shuffle(bool enabled) {
    m_shuffle = enabled;
}

void AudioPipeline::set_repeat(RepeatMode mode) {
    m_repeat = mode;
}

std::chrono::seconds AudioPipeline::current_position() const {
    if (m_bass_stream) {
        double secs = BASS_ChannelBytes2Seconds(m_bass_stream, BASS_ChannelGetPosition(m_bass_stream, BASS_POS_BYTE));
        return std::chrono::seconds(static_cast<int64_t>(secs));
    }
    return std::chrono::seconds(0);
}

void __stdcall AudioPipeline::end_sync_proc(unsigned long, unsigned long, unsigned long, void* user) {
    auto* self = static_cast<AudioPipeline*>(user);
    QMetaObject::invokeMethod(self, "handle_track_ended", Qt::QueuedConnection);
}

void AudioPipeline::handle_track_ended() {
    next();
}

}