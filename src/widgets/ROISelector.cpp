#include "ROISelector.h"
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMouseEvent>
#include <opencv2/imgproc.hpp>

ROISelector::ROISelector(QWidget *parent) : QWidget(parent) {
    m_layout = new QVBoxLayout(this);

    m_dropLabel = new QLabel(tr("将视频文件拖拽到此区域开始分析"), this);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet("QLabel{border: 2px dashed gray; font-size:16px; padding:20px;}");
    m_dropLabel->setMinimumSize(640, 360);
    m_layout->addWidget(m_dropLabel);

    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->hide();
    m_layout->addWidget(m_videoLabel);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->hide();
    m_layout->addWidget(m_slider);
    connect(m_slider, &QSlider::valueChanged, this, &ROISelector::onSliderChanged);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ROISelector::onTimerTick);

    setAcceptDrops(true);
}

void ROISelector::loadVideo(const QString &path) {
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
    m_videoLabel->show();
    m_slider->show();
    m_slider->setRange(0, m_video->getFrameCount() - 1);

    m_frameIndex = 0;
    auto frame = m_video->setAndGetFrame(0);
    if (!frame.empty()) displayFrame(frame);

    m_timer->start(static_cast<int>(1000.0 / m_video->getFrameRate()));
}

void ROISelector::displayFrame(const cv::Mat &frame) {
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
               QImage::Format_RGB888);
    m_videoLabel->setPixmap(QPixmap::fromImage(img.scaled(
        m_videoLabel->size().expandedTo(QSize(640, 360)),
        Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void ROISelector::onTimerTick() {
    if (m_paused || !m_video) return;
    auto frame = m_video->getNextFrame();
    if (frame.empty()) {
        m_timer->stop();
        return;
    }
    ++m_frameIndex;
    displayFrame(frame);
    m_slider->blockSignals(true);
    m_slider->setValue(m_frameIndex);
    m_slider->blockSignals(false);

    if (m_roiSelected && m_processor) {
        cv::Mat roi = frame(cv::Rect(m_roi.x(), m_roi.y(), m_roi.width(), m_roi.height()));
        m_processor->processFrame(roi, m_frameIndex);
    }
}

void ROISelector::onSliderChanged(int value) {
    if (!m_video) return;
    m_frameIndex = value;
    auto frame = m_video->setAndGetFrame(value);
    if (!frame.empty()) displayFrame(frame);
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
        m_paused = !m_paused;
    } else if (event->key() == Qt::Key_S && event->modifiers() & Qt::ControlModifier) {
        if (m_processor) {
            m_paused = true;
            m_processor->saveCSV(m_savePath.toStdString());
            QMessageBox::information(this, tr("保存成功"),
                tr("文件已保存到: %1").arg(m_savePath));
        }
    } else if (event->key() == Qt::Key_Escape) {
        m_roiSelected = false;
        if (m_processor) m_processor->restart();
        update();
    }
    QWidget::keyPressEvent(event);
}

void ROISelector::mousePressEvent(QMouseEvent *event) {
    if (m_paused && !m_roiSelected && event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_selStart = event->pos();
        m_selEnd = m_selStart;
    }
    QWidget::mousePressEvent(event);
}

void ROISelector::mouseMoveEvent(QMouseEvent *event) {
    if (m_selecting) {
        m_selEnd = event->pos();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ROISelector::mouseReleaseEvent(QMouseEvent *event) {
    if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;
        m_selEnd = event->pos();
        m_roi = QRect(m_selStart, m_selEnd).normalized();
        m_roiSelected = true;
        m_paused = false;
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void ROISelector::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    if (m_selecting) {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(QRect(m_selStart, m_selEnd).normalized());
    }
    if (m_roiSelected) {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(m_roi);
    }
}
