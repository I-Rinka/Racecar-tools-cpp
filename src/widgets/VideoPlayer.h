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

private:
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    double m_fps = 30.0;
};
