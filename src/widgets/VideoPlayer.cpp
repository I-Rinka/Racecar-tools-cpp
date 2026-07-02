#include "VideoPlayer.h"
#include <QImage>
#include <QPixmap>
#include <QScrollBar>
#include <QNativeGestureEvent>
#include <QMediaMetaData>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <cmath>

VideoPlayer::VideoPlayer(const QString &videoPath, QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("background: black;");
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_display = new QLabel();
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setScaledContents(false);
    m_display->setStyleSheet("background: black;");
    m_scrollArea->setWidget(m_display);

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

    // Open OpenCV capture for exact frame seeking (used in editor mode)
    m_cvCapture.open(videoPath.toStdString());
    if (m_cvCapture.isOpened()) {
        double fps = m_cvCapture.get(cv::CAP_PROP_FPS);
        if (fps > 0) m_videoFps = fps;
    }

    layout->addWidget(m_scrollArea);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoPlayer::onVideoFrameChanged(const QVideoFrame &frame) {
    QVideoFrame f = frame;
    if (!f.isValid()) return;
    f.map(QVideoFrame::ReadOnly);
    m_lastImage = f.toImage();
    f.unmap();
    if (m_lastImage.isNull()) return;
    updateDisplay();
}

void VideoPlayer::updateDisplay() {
    if (m_lastImage.isNull()) return;
    QSize targetSize = m_scrollArea->size();
    int w = static_cast<int>(targetSize.width() * m_zoomFactor);
    int h = static_cast<int>(targetSize.height() * m_zoomFactor);
    QPixmap pix = QPixmap::fromImage(m_lastImage.scaled(
        QSize(w, h), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_display->setPixmap(pix);
    m_display->resize(pix.size());
}

void VideoPlayer::setZoomEnabled(bool enabled) {
    m_zoomEnabled = enabled;
    if (enabled) {
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        resetZoom();
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void VideoPlayer::resetZoom() {
    m_zoomFactor = 1.0;
    updateDisplay();
}

void VideoPlayer::wheelEvent(QWheelEvent *event) {
    if (!m_zoomEnabled) {
        QWidget::wheelEvent(event);
        return;
    }
    if (event->modifiers() & Qt::ControlModifier) {
        double delta = event->angleDelta().y() > 0 ? 1.15 : 0.87;
        m_zoomFactor = std::clamp(m_zoomFactor * delta, 1.0, 5.0);
        updateDisplay();
        event->accept();
    } else if (event->pixelDelta() != QPoint(0, 0)) {
        // Trackpad pan
        auto *hBar = m_scrollArea->horizontalScrollBar();
        auto *vBar = m_scrollArea->verticalScrollBar();
        hBar->setValue(hBar->value() - event->pixelDelta().x());
        vBar->setValue(vBar->value() - event->pixelDelta().y());
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void VideoPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (!m_mediaReady &&
        (status == QMediaPlayer::LoadedMedia ||
         status == QMediaPlayer::BufferedMedia)) {
        QVariant fpsVar = m_player->metaData().value(QMediaMetaData::VideoFrameRate);
        if (fpsVar.isValid()) {
            double detectedFps = fpsVar.toDouble();
            if (detectedFps > 0) m_videoFps = detectedFps;
        }
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
    if (m_mediaReady) {
        m_player->setPosition(ms);
        m_lastSeekMs = ms;
    } else {
        m_pendingSeekMs = ms;
    }
}

void VideoPlayer::seekToFrame(int frameIndex) {
    qint64 ms = static_cast<qint64>(frameIndex * 1000.0 / m_videoFps);
    if (m_playing) return;
    if (m_mediaReady) {
        m_player->setPosition(ms);
        m_lastSeekMs = ms;
    } else {
        m_pendingSeekMs = ms;
    }
}

void VideoPlayer::seekToFrameExact(int frameIndex) {
    if (!m_cvCapture.isOpened()) {
        seekToFrame(frameIndex);
        return;
    }
    m_cvCapture.set(cv::CAP_PROP_POS_FRAMES, frameIndex);
    cv::Mat mat;
    if (m_cvCapture.read(mat)) {
        QImage img(mat.data, mat.cols, mat.rows,
                   static_cast<int>(mat.step),
                   mat.channels() == 3 ? QImage::Format_BGR888 : QImage::Format_Grayscale8);
        m_lastImage = img.copy();
        updateDisplay();
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

bool VideoPlayer::event(QEvent *ev) {
    if (m_zoomEnabled && ev->type() == QEvent::NativeGesture) {
        auto *ge = static_cast<QNativeGestureEvent *>(ev);
        if (ge->gestureType() == Qt::ZoomNativeGesture) {
            double delta = 1.0 + ge->value();
            m_zoomFactor = std::clamp(m_zoomFactor * delta, 1.0, 5.0);
            updateDisplay();
            return true;
        }
    }
    return QWidget::event(ev);
}
