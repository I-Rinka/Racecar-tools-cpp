#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSlider>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QThread>
#include <opencv2/core.hpp>
#include "core/VideoWrapper.h"
#include "core/VideoProcessor.h"

class ROISelector : public QWidget {
    Q_OBJECT
public:
    explicit ROISelector(QWidget *parent = nullptr);
    void loadVideo(const QString &path);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTimerTick();
    void onSliderChanged(int value);

private:
    QLabel *m_dropLabel;
    QLabel *m_videoLabel;
    QSlider *m_slider;
    QTimer *m_timer;
    QVBoxLayout *m_layout;

    VideoWrapper *m_video = nullptr;
    VideoProcessor *m_processor = nullptr;
    int m_frameIndex = 0;
    bool m_paused = false;
    bool m_roiSelected = false;
    QRect m_roi;
    QPoint m_selStart, m_selEnd;
    bool m_selecting = false;
    QString m_savePath;

    void displayFrame(const cv::Mat &frame);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};
