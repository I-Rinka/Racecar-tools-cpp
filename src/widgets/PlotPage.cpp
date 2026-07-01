#include "PlotPage.h"
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDragEnterEvent>

static const QColor COLORS[] = {
    QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#d62728"), QColor("#9467bd")
};

PlotPage::PlotPage(QWidget *parent) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);

    m_plot = new PlotWidget(this);
    m_plot->setMinimumHeight(200);
    mainLayout->addWidget(m_plot, 1);

    m_videoContainer = new QWidget(this);
    m_videoLayout = new QGridLayout(m_videoContainer);
    mainLayout->addWidget(m_videoContainer, 2);

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

void PlotPage::addInstance(const QString &csvPath, const QString &videoPath) {
    auto *analyzer = new SDAnalyzer(csvPath.toStdString());
    m_analyzers.push_back(analyzer);

    QColor color = COLORS[(m_analyzers.size() - 1) % 4];
    m_plot->addAnalyzer(analyzer, color);

    auto *player = new VideoPlayer(videoPath, this);
    m_videos.push_back(player);
    m_videoFps.push_back(30.0);

    int initFrame = analyzer->getInitialFrame();
    player->seekToFrame(initFrame, 30.0);

    registerVideoHover(static_cast<int>(m_videos.size()) - 1);
    refreshVideoLayout();
}

void PlotPage::addInstanceByData(const QString &name, const SpeedData &data,
                                  const QString &videoPath) {
    auto *analyzer = new SDAnalyzer(name.toStdString(), data);
    m_analyzers.push_back(analyzer);

    QColor color = COLORS[(m_analyzers.size() - 1) % 4];
    m_plot->addAnalyzer(analyzer, color);

    auto *player = new VideoPlayer(videoPath, this);
    m_videos.push_back(player);
    m_videoFps.push_back(30.0);

    int initFrame = analyzer->getInitialFrame();
    player->seekToFrame(initFrame, 30.0);

    registerVideoHover(static_cast<int>(m_videos.size()) - 1);
    refreshVideoLayout();
}

void PlotPage::registerVideoHover(int index) {
    m_plot->registerHoverCallback(index, [this, index](SDAnalyzer *a, int) {
        if (index < static_cast<int>(m_videos.size())) {
            int frame = a->getCurrentFrameIndex();
            double fps = index < static_cast<int>(m_videoFps.size()) ? m_videoFps[index] : 30.0;
            m_videos[index]->seekToFrame(frame, fps);
        }
    });
}

void PlotPage::refreshVideoLayout() {
    while (m_videoLayout->count() > 0) {
        auto *item = m_videoLayout->takeAt(0);
        if (item->widget()) item->widget()->setParent(nullptr);
        delete item;
    }

    int count = static_cast<int>(m_videos.size());
    if (count == 0) return;
    int cols = (count >= 2) ? 2 : 1;
    for (int i = 0; i < count; ++i) {
        int row = i / cols, col = i % cols;
        m_videoLayout->addWidget(m_videos[i], row, col);
    }
    int rows = (count + cols - 1) / cols;
    for (int r = 0; r < rows; ++r) m_videoLayout->setRowStretch(r, 1);
    for (int c = 0; c < cols; ++c) m_videoLayout->setColumnStretch(c, 1);
}

void PlotPage::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PlotPage::dropEvent(QDropEvent *event) {
    for (auto &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        QFileInfo fi(path);
        QString ext = fi.suffix().toLower();

        if (ext == "csv") {
            QString base = fi.completeBaseName();
            if (base.endsWith("_database"))
                base = base.left(base.length() - 9);
            QString videoPath = fi.dir().filePath(base + ".mp4");
            if (QFileInfo::exists(videoPath)) {
                addInstance(path, videoPath);
            } else {
                m_pendingCsv = path;
                auto *analyzer = new SDAnalyzer(path.toStdString());
                m_analyzers.push_back(analyzer);
                QColor color = COLORS[(m_analyzers.size() - 1) % 4];
                m_plot->addAnalyzer(analyzer, color);
                QMessageBox::information(this, tr("提示"),
                    tr("CSV 已加载，请拖入对应 MP4 文件配对"));
            }
        } else if (ext == "mp4") {
            if (m_pendingCsv.isEmpty()) {
                QMessageBox::warning(this, tr("错误"), tr("请先拖入 CSV 文件"));
            } else {
                auto *player = new VideoPlayer(path, this);
                m_videos.push_back(player);
                m_videoFps.push_back(30.0);
                int idx = static_cast<int>(m_analyzers.size()) - 1;
                if (idx >= 0) {
                    player->seekToFrame(m_analyzers[idx]->getInitialFrame(), 30.0);
                    registerVideoHover(static_cast<int>(m_videos.size()) - 1);
                }
                refreshVideoLayout();
                m_pendingCsv.clear();
            }
        }
    }
}

void PlotPage::keyPressEvent(QKeyEvent *event) {
    // Space = toggle play/animation
    if (event->key() == Qt::Key_Space) {
        m_playing = !m_playing;
        for (auto *v : m_videos) v->setPlaying(m_playing);
        if (m_playing)
            m_plot->startAnimation();
        else
            m_plot->stopAnimation();
    }

    // Left/Right = adjust selected curve distance by 1m
    int sel = m_plot->selectedIndex();
    if (sel >= 0 && sel < static_cast<int>(m_analyzers.size())) {
        if (event->key() == Qt::Key_Left) {
            m_analyzers[sel]->adjustDistance(-1.0);
            m_plot->refreshGraphData();
        } else if (event->key() == Qt::Key_Right) {
            m_analyzers[sel]->adjustDistance(1.0);
            m_plot->refreshGraphData();
        }
    }

    // Escape = clear delta texts
    if (event->key() == Qt::Key_Escape) {
        // deselect
        m_plot->setSelectedIndex(-1);
    }

    // Ctrl state
    if (event->key() == Qt::Key_Control) {
        m_plot->setCtrlPressed(true);
    }

    QWidget::keyPressEvent(event);
}

void PlotPage::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        m_plot->setCtrlPressed(false);
    }
    QWidget::keyReleaseEvent(event);
}
