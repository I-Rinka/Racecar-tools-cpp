#include "VideoPlayer.h"

VideoPlayer::VideoPlayer(const QString &videoPath, QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_videoWidget = new QVideoWidget(this);
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);
    m_player->setSource(QUrl::fromLocalFile(videoPath));

    layout->addWidget(m_videoWidget);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoPlayer::seekToFrame(int frameIndex, double fps) {
    m_fps = fps;
    qint64 ms = static_cast<qint64>(frameIndex * 1000.0 / fps);
    m_player->setPosition(ms);
}

void VideoPlayer::setPlaying(bool playing) {
    if (playing)
        m_player->play();
    else
        m_player->pause();
}

void VideoPlayer::seekToMs(qint64 ms) {
    m_player->setPosition(ms);
}
