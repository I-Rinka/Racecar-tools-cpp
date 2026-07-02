#include "VideoPlayer.h"
#include <QImage>
#include <QPixmap>

VideoPlayer::VideoPlayer(const QString &videoPath, QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_display = new QLabel(this);
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setScaledContents(false);
    m_display->setStyleSheet("background: black;");

    m_videoSink = new QVideoSink(this);
    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &VideoPlayer::onVideoFrameChanged);

    m_player = new QMediaPlayer(this);
    m_player->setVideoSink(m_videoSink);

    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &VideoPlayer::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &VideoPlayer::onPlaybackStateChanged);

    m_player->setSource(QUrl::fromLocalFile(videoPath));

    layout->addWidget(m_display);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoPlayer::onVideoFrameChanged(const QVideoFrame &frame) {
    QVideoFrame f = frame;
    if (!f.isValid()) return;
    f.map(QVideoFrame::ReadOnly);
    QImage img = f.toImage();
    f.unmap();
    if (img.isNull()) return;
    QPixmap pix = QPixmap::fromImage(img.scaled(
        m_display->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_display->setPixmap(pix);
}

void VideoPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (!m_mediaReady &&
        (status == QMediaPlayer::LoadedMedia ||
         status == QMediaPlayer::BufferedMedia)) {
        m_initializing = true;
        m_player->play();
    }
}

void VideoPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    if (m_initializing && state == QMediaPlayer::PlayingState) {
        m_initializing = false;
        m_mediaReady = true;
        if (m_wantPlay) {
            if (m_pendingSeekMs >= 0) {
                m_player->setPosition(m_pendingSeekMs);
                m_lastSeekMs = m_pendingSeekMs;
                m_pendingSeekMs = -1;
            }
            return;
        }
        m_player->pause();
        if (m_pendingSeekMs >= 0) {
            m_player->setPosition(m_pendingSeekMs);
            m_lastSeekMs = m_pendingSeekMs;
            m_pendingSeekMs = -1;
        }
    }
}

void VideoPlayer::seekToFrame(int frameIndex, double fps) {
    m_fps = fps;
    qint64 ms = static_cast<qint64>(frameIndex * 1000.0 / fps);
    if (m_playing) return;
    if (m_mediaReady && std::abs(ms - m_lastSeekMs) < 30) return;
    if (m_mediaReady) {
        m_player->setPosition(ms);
        m_lastSeekMs = ms;
    } else {
        m_pendingSeekMs = ms;
    }
}

void VideoPlayer::setPlaying(bool playing) {
    m_playing = playing;
    if (playing) {
        m_wantPlay = true;
        if (m_mediaReady) {
            m_player->play();
        }
    } else {
        m_wantPlay = false;
        if (m_mediaReady) {
            m_player->pause();
            m_lastSeekMs = m_player->position();
        }
    }
}

void VideoPlayer::seekToMs(qint64 ms) {
    if (m_mediaReady) {
        m_player->setPosition(ms);
        m_lastSeekMs = ms;
    } else {
        m_pendingSeekMs = ms;
    }
}

qint64 VideoPlayer::position() const {
    return m_player->position();
}
