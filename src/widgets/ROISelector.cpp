#include "ROISelector.h"
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMouseEvent>
#include <opencv2/imgproc.hpp>

// ==================== VideoCanvas ====================

VideoCanvas::VideoCanvas(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setScaledContents(false);
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VideoCanvas::setFrame(const cv::Mat &frame) {
    if (frame.empty()) return;
    m_frame = frame.clone();
    cv::Mat rgb;
    cv::cvtColor(m_frame, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
               QImage::Format_RGB888);
    m_pixmap = QPixmap::fromImage(img.copy());
    setPixmap(m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    update();
}

void VideoCanvas::clearROI() {
    m_roiSelected = false;
    m_roiImage = QRect();
    m_selecting = false;
    update();
}

QRect VideoCanvas::imageDisplayRect() const {
    if (m_frame.empty()) return QRect();
    int imgW = m_frame.cols, imgH = m_frame.rows;
    int labelW = width(), labelH = height();
    double scale = std::min(double(labelW) / imgW, double(labelH) / imgH);
    int dispW = int(imgW * scale), dispH = int(imgH * scale);
    int offX = (labelW - dispW) / 2, offY = (labelH - dispH) / 2;
    return QRect(offX, offY, dispW, dispH);
}

QPoint VideoCanvas::mapToImage(const QPoint &pos) const {
    if (m_frame.empty()) return QPoint(0, 0);
    QRect disp = imageDisplayRect();
    if (disp.width() == 0 || disp.height() == 0) return QPoint(0, 0);

    int x = std::clamp(pos.x(), disp.left(), disp.right());
    int y = std::clamp(pos.y(), disp.top(), disp.bottom());
    int imgX = (x - disp.left()) * m_frame.cols / disp.width();
    int imgY = (y - disp.top()) * m_frame.rows / disp.height();
    imgX = std::clamp(imgX, 0, m_frame.cols - 1);
    imgY = std::clamp(imgY, 0, m_frame.rows - 1);
    return QPoint(imgX, imgY);
}

QPoint VideoCanvas::mapFromImage(const QPoint &imgPt) const {
    if (m_frame.empty()) return QPoint(0, 0);
    QRect disp = imageDisplayRect();
    int x = imgPt.x() * disp.width() / m_frame.cols + disp.left();
    int y = imgPt.y() * disp.height() / m_frame.rows + disp.top();
    return QPoint(x, y);
}

void VideoCanvas::mousePressEvent(QMouseEvent *event) {
    if (m_canSelect && !m_roiSelected && event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_selStart = mapToImage(event->pos());
        m_selEnd = m_selStart;
        update();
    }
    QLabel::mousePressEvent(event);
}

void VideoCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (m_selecting) {
        m_selEnd = mapToImage(event->pos());
        update();
    }
    QLabel::mouseMoveEvent(event);
}

void VideoCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;
        m_selEnd = mapToImage(event->pos());
        int x1 = std::min(m_selStart.x(), m_selEnd.x());
        int y1 = std::min(m_selStart.y(), m_selEnd.y());
        int x2 = std::max(m_selStart.x(), m_selEnd.x());
        int y2 = std::max(m_selStart.y(), m_selEnd.y());
        int w = std::max(1, x2 - x1), h = std::max(1, y2 - y1);
        m_roiImage = QRect(x1, y1, w, h);
        m_roiSelected = true;
        m_canSelect = false;
        update();
        emit roiFinished(m_roiImage);
    }
    QLabel::mouseReleaseEvent(event);
}

void VideoCanvas::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    if (!m_pixmap.isNull())
        setPixmap(m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void VideoCanvas::paintEvent(QPaintEvent *event) {
    QLabel::paintEvent(event);
    if (m_frame.empty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::red, 2);

    if (m_selecting) {
        QPoint p1 = mapFromImage(m_selStart);
        QPoint p2 = mapFromImage(m_selEnd);
        painter.setPen(pen);
        painter.drawRect(QRect(p1, p2).normalized());
    }

    if (m_roiSelected && m_roiImage.isValid()) {
        QRect disp = imageDisplayRect();
        QColor maskColor(0, 0, 0, 160);
        painter.fillRect(disp, maskColor);

        QPoint tl = mapFromImage(m_roiImage.topLeft());
        QPoint br = mapFromImage(m_roiImage.bottomRight());
        QRect drawRect(tl, br);
        drawRect = drawRect.normalized();

        // draw the ROI region bright
        cv::Mat roi = m_frame(cv::Rect(m_roiImage.x(), m_roiImage.y(),
                                        m_roiImage.width(), m_roiImage.height()));
        cv::Mat roiRgb;
        cv::cvtColor(roi, roiRgb, cv::COLOR_BGR2RGB);
        QImage roiImg(roiRgb.data, roiRgb.cols, roiRgb.rows,
                      static_cast<int>(roiRgb.step), QImage::Format_RGB888);
        painter.drawImage(drawRect, roiImg.copy());
        painter.setPen(pen);
        painter.drawRect(drawRect);

        painter.setPen(Qt::white);
        painter.drawText(disp.left() + 6, disp.top() + 16,
                         QString("ROI: x=%1 y=%2 w=%3 h=%4")
                             .arg(m_roiImage.x()).arg(m_roiImage.y())
                             .arg(m_roiImage.width()).arg(m_roiImage.height()));
    }
}

// ==================== ROISelector ====================

ROISelector::ROISelector(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);

    m_dropLabel = new QLabel(tr("将视频文件拖拽到此区域开始分析"), this);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet("QLabel{border: 2px dashed gray; font-size:16px; padding:20px;}");
    m_dropLabel->setMinimumSize(640, 360);
    m_layout->addWidget(m_dropLabel);

    m_canvas = new VideoCanvas(this);
    m_canvas->hide();
    m_layout->addWidget(m_canvas, 1);
    connect(m_canvas, &VideoCanvas::roiFinished, this, &ROISelector::onROIFinished);

    // control bar
    m_controlBar = new QWidget(this);
    auto *barLayout = new QHBoxLayout(m_controlBar);
    barLayout->setContentsMargins(0, 0, 0, 0);

    m_stepBwdBtn = new QPushButton("◀◀", this);
    m_stepBwdBtn->setFixedSize(50, 30);
    connect(m_stepBwdBtn, &QPushButton::clicked, this, &ROISelector::stepBackward);
    barLayout->addWidget(m_stepBwdBtn);

    m_playBtn = new QPushButton(tr("▶ 播放"), this);
    m_playBtn->setFixedSize(80, 30);
    connect(m_playBtn, &QPushButton::clicked, this, &ROISelector::togglePlayPause);
    barLayout->addWidget(m_playBtn);

    m_stepFwdBtn = new QPushButton("▶▶", this);
    m_stepFwdBtn->setFixedSize(50, 30);
    connect(m_stepFwdBtn, &QPushButton::clicked, this, &ROISelector::stepForward);
    barLayout->addWidget(m_stepFwdBtn);

    m_statusLabel = new QLabel(tr("就绪"), this);
    barLayout->addStretch();
    barLayout->addWidget(m_statusLabel);

    m_controlBar->hide();
    m_layout->addWidget(m_controlBar);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->hide();
    m_layout->addWidget(m_slider);
    connect(m_slider, &QSlider::valueChanged, this, &ROISelector::onSliderChanged);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ROISelector::onTimerTick);

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

void ROISelector::loadVideo(const QString &path) {
    m_videoPath = path;
    m_video = new VideoWrapper(path.toStdString());
    if (!m_video->isOpened()) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开视频文件"));
        delete m_video;
        m_video = nullptr;
        return;
    }

    m_processor = new VideoProcessor(m_video->getFrameRate());

    QFileInfo fi(path);
    m_savePath = fi.dir().filePath(fi.completeBaseName() + "_database.csv");

    m_dropLabel->hide();
    m_canvas->show();
    m_controlBar->show();
    m_slider->show();
    m_slider->setRange(0, m_video->getFrameCount() - 1);

    m_frameIndex = 0;
    displayFrameAt(0);

    // start playing
    m_paused = false;
    m_playBtn->setText(tr("⏸ 暂停"));
    m_statusLabel->setText(tr("播放中 - 按空格暂停后框选速度区域"));
    m_timer->start(static_cast<int>(1000.0 / m_video->getFrameRate()));
}

void ROISelector::displayFrameAt(int index) {
    if (!m_video) return;
    auto frame = m_video->setAndGetFrame(index);
    if (!frame.empty()) {
        m_frameIndex = index;
        m_canvas->setFrame(frame);
        m_slider->blockSignals(true);
        m_slider->setValue(index);
        m_slider->blockSignals(false);
    }
}

void ROISelector::onTimerTick() {
    if (m_paused || !m_video) return;

    auto frame = m_video->getNextFrame();
    if (frame.empty()) {
        m_timer->stop();
        m_statusLabel->setText(tr("视频结束"));
        return;
    }
    ++m_frameIndex;
    m_canvas->setFrame(frame);
    m_slider->blockSignals(true);
    m_slider->setValue(m_frameIndex);
    m_slider->blockSignals(false);

    if (m_processing && m_roiSelected && m_processor) {
        QRect r = m_roiRect;
        cv::Mat roi = frame(cv::Rect(r.x(), r.y(), r.width(), r.height()));
        double speed = m_processor->processFrame(roi, m_frameIndex);
        m_statusLabel->setText(tr("识别中... 帧 %1, 速度: %2 km/h")
                                   .arg(m_frameIndex).arg(speed, 0, 'f', 1));
    }
}

void ROISelector::onSliderChanged(int value) {
    displayFrameAt(value);
}

void ROISelector::togglePlayPause() {
    if (!m_video) return;
    m_paused = !m_paused;
    if (m_paused) {
        m_timer->stop();
        m_playBtn->setText(tr("▶ 播放"));
        if (!m_roiSelected) {
            m_canvas->setCanSelect(true);
            m_statusLabel->setText(tr("已暂停 - 用鼠标框选速度数字区域"));
        } else {
            stopProcessing();
            m_statusLabel->setText(tr("已暂停"));
        }
    } else {
        m_canvas->setCanSelect(false);
        m_playBtn->setText(tr("⏸ 暂停"));
        if (m_roiSelected) {
            startProcessing();
            m_statusLabel->setText(tr("识别中..."));
        } else {
            m_statusLabel->setText(tr("播放中 - 按空格暂停后框选速度区域"));
        }
        m_timer->start(static_cast<int>(1000.0 / m_video->getFrameRate()));
    }
}

void ROISelector::stepForward() {
    if (!m_video) return;
    m_paused = true;
    m_timer->stop();
    m_playBtn->setText(tr("▶ 播放"));
    displayFrameAt(m_frameIndex + 1);
}

void ROISelector::stepBackward() {
    if (!m_video) return;
    m_paused = true;
    m_timer->stop();
    m_playBtn->setText(tr("▶ 播放"));
    if (m_frameIndex > 0)
        displayFrameAt(m_frameIndex - 1);
}

void ROISelector::onROIFinished(QRect roi) {
    m_roiRect = roi;
    m_roiSelected = true;
    m_statusLabel->setText(tr("ROI 已选定 - 按 ▶ 开始识别，Esc 清除"));
}

void ROISelector::startProcessing() {
    m_processing = true;
}

void ROISelector::stopProcessing() {
    m_processing = false;
}

SpeedData ROISelector::getResult() {
    if (m_processor) return m_processor->getSpeedData();
    return {};
}

void ROISelector::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void ROISelector::dropEvent(QDropEvent *event) {
    for (auto &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (path.endsWith(".mp4", Qt::CaseInsensitive)) {
            loadVideo(path);
            return;
        }
    }
}

void ROISelector::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        togglePlayPause();
    } else if (event->key() == Qt::Key_Left) {
        stepBackward();
    } else if (event->key() == Qt::Key_Right) {
        stepForward();
    } else if (event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier) {
        if (m_processor) {
            m_paused = true;
            m_timer->stop();
            m_playBtn->setText(tr("▶ 播放"));
            if (m_roiSelected) {
                m_statusLabel->setText(tr("正在优化毛刺数据..."));
                m_statusLabel->repaint();
                m_processor->refineSpikes(m_videoPath.toStdString(),
                    cv::Rect(m_roiRect.x(), m_roiRect.y(),
                             m_roiRect.width(), m_roiRect.height()));
            }
            m_processor->saveCSV(m_savePath.toStdString());
            m_statusLabel->setText(tr("已保存"));
            QMessageBox::information(this, tr("保存成功"),
                tr("文件已保存到: %1").arg(m_savePath));
            emit processingFinished(
                QFileInfo(m_savePath).completeBaseName(),
                m_processor->getSpeedData(), m_videoPath);
        }
    } else if (event->key() == Qt::Key_Escape) {
        m_roiSelected = false;
        m_processing = false;
        m_canvas->clearROI();
        if (m_processor) m_processor->restart();
        m_statusLabel->setText(tr("ROI 已清除"));
    }
    QWidget::keyPressEvent(event);
}
