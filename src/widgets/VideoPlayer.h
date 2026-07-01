#pragma once
#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
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

private:
    void doSeek(qint64 ms);

    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    double m_fps = 30.0;
    bool m_playing = false;
    bool m_mediaReady = false;
    bool m_pendingPlay = false;
    qint64 m_lastSeekMs = -1;
    qint64 m_pendingSeekMs = -1;
};
