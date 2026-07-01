#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QPushButton>
#include <QThread>
#include <opencv2/core.hpp>
#include "core/VideoWrapper.h"
#include "core/VideoProcessor.h"

class VideoCanvas : public QLabel {
    Q_OBJECT
public:
    explicit VideoCanvas(QWidget *parent = nullptr);

    void setFrame(const cv::Mat &frame);
    void setCanSelect(bool enabled) { m_canSelect = enabled; }
    bool roiSelected() const { return m_roiSelected; }
    QRect roi() const { return m_roiImage; }
    void clearROI();

    const cv::Mat &currentFrame() const { return m_frame; }

signals:
    void roiFinished(QRect roiInImage);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPoint mapToImage(const QPoint &widgetPos) const;
    QPoint mapFromImage(const QPoint &imagePos) const;
    QRect imageDisplayRect() const;

    cv::Mat m_frame;
    QPixmap m_pixmap;
    bool m_canSelect = false;
    bool m_roiSelected = false;
    bool m_selecting = false;
    QPoint m_selStart, m_selEnd;
    QRect m_roiImage;
};

class ROISelector : public QWidget {
    Q_OBJECT
public:
    explicit ROISelector(QWidget *parent = nullptr);
    void loadVideo(const QString &path);
    SpeedData getResult();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTimerTick();
    void onSliderChanged(int value);
    void onROIFinished(QRect roi);
    void togglePlayPause();
    void stepForward();
    void stepBackward();

private:
    void displayFrameAt(int index);
    void startProcessing();
    void stopProcessing();

    VideoCanvas *m_canvas;
    QLabel *m_dropLabel;
    QSlider *m_slider;
    QTimer *m_timer;
    QVBoxLayout *m_layout;
    QPushButton *m_playBtn;
    QPushButton *m_stepFwdBtn;
    QPushButton *m_stepBwdBtn;
    QLabel *m_statusLabel;
    QWidget *m_controlBar;

    VideoWrapper *m_video = nullptr;
    VideoProcessor *m_processor = nullptr;
    int m_frameIndex = 0;
    bool m_paused = true;
    bool m_roiSelected = false;
    QRect m_roiRect;
    QString m_savePath;
    bool m_processing = false;
};
