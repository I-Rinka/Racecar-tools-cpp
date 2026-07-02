#pragma once
#include <QWidget>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>

class VideoPlayer : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayer(const QString &videoPath, QWidget *parent = nullptr);

    void seekToFrame(int frameIndex, double fps);
    void setPlaying(bool playing);
    void seekToMs(qint64 ms);
    qint64 position() const;
    bool isPlaying() const { return m_playing; }

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onVideoFrameChanged(const QVideoFrame &frame);

private:
    QMediaPlayer *m_player;
    QVideoSink *m_videoSink;
    QLabel *m_display;
    double m_fps = 30.0;
    bool m_playing = false;
    bool m_mediaReady = false;
    bool m_initializing = false;
    bool m_wantPlay = false;
    qint64 m_lastSeekMs = -1;
    qint64 m_pendingSeekMs = -1;
};
