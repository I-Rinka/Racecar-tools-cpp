#pragma once
#include <QWidget>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QWheelEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <opencv2/videoio.hpp>

class VideoPlayer : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayer(const QString &videoPath, QWidget *parent = nullptr);

    void seekToFrame(int frameIndex, double fps);
    void seekToFrame(int frameIndex);
    void seekToFrameExact(int frameIndex);
    void setPlaying(bool playing);
    void seekToMs(qint64 ms);
    qint64 position() const;
    bool isPlaying() const { return m_playing; }
    double videoFps() const { return m_videoFps; }

    void setZoomEnabled(bool enabled);
    void resetZoom();

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onVideoFrameChanged(const QVideoFrame &frame);

protected:
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;

private:
    void updateDisplay();

    QMediaPlayer *m_player;
    QVideoSink *m_videoSink;
    QLabel *m_display;
    QScrollArea *m_scrollArea = nullptr;
    double m_fps = 30.0;
    double m_videoFps = 30.0;
    bool m_playing = false;
    bool m_mediaReady = false;
    bool m_initializing = false;
    bool m_wantPlay = false;
    qint64 m_lastSeekMs = -1;
    qint64 m_pendingSeekMs = -1;

    // Zoom state
    bool m_zoomEnabled = false;
    double m_zoomFactor = 1.0;
    QImage m_lastImage;

    // OpenCV capture for exact frame seeking
    cv::VideoCapture m_cvCapture;
};
