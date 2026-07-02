#include "PlotPage.h"
#include "ROISelector.h"
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <numeric>

static const QColor COLORS[] = {
    QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#d62728"), QColor("#9467bd")
};

PlotPage::PlotPage(QWidget *parent) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);

    m_plot = new PlotWidget(this);
    m_plot->setMinimumHeight(200);
    mainLayout->addWidget(m_plot, 1);

    m_videoContainer = new QWidget(this);
    m_videoLayout = new QHBoxLayout(m_videoContainer);
    m_videoLayout->setContentsMargins(0, 0, 0, 0);
    m_videoLayout->setSpacing(4);
    mainLayout->addWidget(m_videoContainer, 2);

    m_editBtn = new QPushButton(tr("改变曲线"), this);
    m_editBtn->setFixedHeight(28);
    m_editBtn->setMaximumWidth(100);
    connect(m_editBtn, &QPushButton::clicked, this, &PlotPage::onEditCurve);
    mainLayout->addWidget(m_editBtn, 0, Qt::AlignLeft);

    m_syncTimer = new QTimer(this);
    connect(m_syncTimer, &QTimer::timeout, this, &PlotPage::onSyncTick);

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
}

void PlotPage::addInstance(const QString &csvPath, const QString &videoPath) {
    auto *analyzer = new SDAnalyzer(csvPath.toStdString());
    m_analyzers.push_back(analyzer);

    QColor color = COLORS[(m_analyzers.size() - 1) % 4];
    m_plot->addAnalyzer(analyzer, color);

    double fps = analyzer->getFps();

    auto *player = new VideoPlayer(videoPath, this);
    m_videos.push_back(player);
    m_videoFps.push_back(fps);

    int initFrame = analyzer->getInitialFrame();
    player->seekToFrame(initFrame, fps);

    registerVideoHover(static_cast<int>(m_videos.size()) - 1);
    refreshVideoLayout();
}

void PlotPage::addInstanceByData(const QString &name, const SpeedData &data,
                                  const QString &videoPath) {
    auto *analyzer = new SDAnalyzer(name.toStdString(), data);
    m_analyzers.push_back(analyzer);

    QColor color = COLORS[(m_analyzers.size() - 1) % 4];
    m_plot->addAnalyzer(analyzer, color);

    double fps = analyzer->getFps();

    auto *player = new VideoPlayer(videoPath, this);
    m_videos.push_back(player);
    m_videoFps.push_back(fps);

    int initFrame = analyzer->getInitialFrame();
    player->seekToFrame(initFrame, fps);

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

    for (auto *v : m_videos) {
        v->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        v->setMinimumSize(0, 0);
        m_videoLayout->addWidget(v, 1);
    }
}

void PlotPage::onSyncTick() {
    for (size_t i = 0; i < m_videos.size() && i < m_analyzers.size(); ++i) {
        qint64 posMs = m_videos[i]->position();
        double fps = (i < m_videoFps.size()) ? m_videoFps[i] : 30.0;
        int frame = static_cast<int>(posMs * fps / 1000.0);
        m_analyzers[i]->setCurrentIndexByFrame(frame);
    }
    m_plot->updatePlot();

    if (m_analyzers.size() >= 2) {
        double d0 = m_analyzers[0]->getCurrentDistance();
        double d1 = m_analyzers[1]->getCurrentDistance();
        m_plot->showDistanceDiff(d0, d1);
    }
}

void PlotPage::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls() ||
        event->mimeData()->hasFormat("application/x-racecar-tab-index"))
        event->acceptProposedAction();
}

void PlotPage::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-racecar-tab-index")) {
        int tabIdx = event->mimeData()->data("application/x-racecar-tab-index").toInt();
        emit roiTabDropped(tabIdx);
        event->acceptProposedAction();
        return;
    }

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
                int idx = static_cast<int>(m_analyzers.size()) - 1;
                double fps = (idx >= 0) ? m_analyzers[idx]->getFps() : 30.0;
                auto *player = new VideoPlayer(path, this);
                m_videos.push_back(player);
                m_videoFps.push_back(fps);
                if (idx >= 0) {
                    player->seekToFrame(m_analyzers[idx]->getInitialFrame(), fps);
                    registerVideoHover(static_cast<int>(m_videos.size()) - 1);
                }
                refreshVideoLayout();
                m_pendingCsv.clear();
            }
        }
    }
}

void PlotPage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        m_playing = !m_playing;
        if (m_playing) {
            for (size_t i = 0; i < m_videos.size(); ++i) {
                if (i < m_analyzers.size()) {
                    int frame = m_analyzers[i]->getCurrentFrameIndex();
                    double fps = i < m_videoFps.size() ? m_videoFps[i] : 30.0;
                    qint64 ms = static_cast<qint64>(frame * 1000.0 / fps);
                    m_videos[i]->seekToMs(ms);
                }
                m_videos[i]->setPlaying(true);
            }
            m_syncTimer->start(33);
        } else {
            for (auto *v : m_videos) v->setPlaying(false);
            m_syncTimer->stop();
            m_plot->hideDistanceDiff();
        }
    }

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

    if (event->key() == Qt::Key_Escape) {
        if (m_plot->selectedIndex() >= 0) {
            m_plot->setSelectedIndex(-1);
        } else {
            m_plot->clearDeltaTexts();
        }
    }

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

void PlotPage::onEditCurve() {
    int sel = m_plot->selectedIndex();
    if (sel < 0 || sel >= static_cast<int>(m_analyzers.size())) {
        QMessageBox::information(this, tr("提示"), tr("请先点击选中一条曲线"));
        return;
    }

    m_editingIndex = sel;
    auto *analyzer = m_analyzers[sel];
    SpeedData &data = const_cast<SpeedData &>(analyzer->data());

    std::vector<int> indices(data.size());
    std::iota(indices.begin(), indices.end(), 0);

    m_videoContainer->hide();
    m_editBtn->hide();

    m_editorContainer = new QWidget(this);
    auto *editorLayout = new QHBoxLayout(m_editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);

    m_editor = new DataEditor(data, indices,
        [this](const SpeedData &) { onEditorClosed(); }, this);

    // Video for this curve (reuse path from existing player or create new)
    QString videoPath;
    if (sel < static_cast<int>(m_videos.size())) {
        // Get the video source — we need to create a new player for the editor
        // The video path isn't stored separately, so reuse the existing player
        m_editorVideo = m_videos[sel];
        m_editorVideo->setParent(m_editorContainer);
    } else {
        m_editorVideo = nullptr;
    }

    editorLayout->addWidget(m_editor, 3);
    if (m_editorVideo)
        editorLayout->addWidget(m_editorVideo, 2);

    // Insert editor container where video container was
    auto *mainLayout = qobject_cast<QVBoxLayout *>(layout());
    mainLayout->insertWidget(1, m_editorContainer, 2);

    double fps = (sel < static_cast<int>(m_videoFps.size())) ? m_videoFps[sel] : 30.0;
    m_editor->registerHoverCallback([this, fps](int idx) {
        if (!m_editorVideo) return;
        auto *analyzer = m_analyzers[m_editingIndex];
        int frame = analyzer->data().frame[idx];
        m_editorVideo->seekToFrame(frame, fps);
    });
}

void PlotPage::onEditorClosed() {
    if (m_editingIndex >= 0 && m_editingIndex < static_cast<int>(m_analyzers.size())) {
        auto &data = m_analyzers[m_editingIndex]->data();
        // Recompute distance from speed after edits
        if (!data.speed.empty() && !data.time_s.empty()) {
            data.distance.resize(data.speed.size());
            data.distance[0] = 0;
            for (int i = 1; i < data.size(); ++i) {
                double dt = data.time_s[i] - data.time_s[i - 1];
                double vavg = (data.speed[i] + data.speed[i - 1]) / 2.0;
                data.distance[i] = data.distance[i - 1] + vavg * dt * 1000.0 / 3600.0;
            }
        }
        m_analyzers[m_editingIndex]->replaceData(data);
        m_plot->refreshGraphData();
        m_plot->rescaleAxes();
        m_plot->replot();
    }

    if (m_editorVideo && m_editingIndex < static_cast<int>(m_videos.size())) {
        m_editorVideo->setParent(m_videoContainer);
        m_editorVideo = nullptr;
    }

    if (m_editorContainer) {
        auto *mainLayout = qobject_cast<QVBoxLayout *>(layout());
        mainLayout->removeWidget(m_editorContainer);
        m_editorContainer->deleteLater();
        m_editorContainer = nullptr;
        m_editor = nullptr;
    }

    m_videoContainer->show();
    m_editBtn->show();
    refreshVideoLayout();
    m_editingIndex = -1;
}

void PlotPage::addLiveInstance(ROISelector *roi) {
    SpeedData data = roi->getResult();
    QString name = QFileInfo(roi->videoPath()).completeBaseName();

    auto *analyzer = new SDAnalyzer(name.toStdString(), data);
    m_analyzers.push_back(analyzer);

    QColor color = COLORS[(m_analyzers.size() - 1) % 4];
    m_plot->addAnalyzer(analyzer, color);

    QString videoPath = roi->videoPath();
    if (!videoPath.isEmpty()) {
        double fps = analyzer->getFps();
        auto *player = new VideoPlayer(videoPath, this);
        m_videos.push_back(player);
        m_videoFps.push_back(fps);
        int initFrame = analyzer->getInitialFrame();
        player->seekToFrame(initFrame, fps);
        registerVideoHover(static_cast<int>(m_videos.size()) - 1);
        refreshVideoLayout();
    }

    if (roi->isProcessing()) {
        LiveSource src;
        src.roi = roi;
        src.analyzerIndex = static_cast<int>(m_analyzers.size()) - 1;
        src.fullData = data;
        src.lastSize = data.size();
        m_liveSources.push_back(std::move(src));
        if (!m_liveTimer) {
            m_liveTimer = new QTimer(this);
            connect(m_liveTimer, &QTimer::timeout, this, &PlotPage::onLiveUpdate);
        }
        m_liveInterval = 100;
        m_liveTimer->start(m_liveInterval);
    }
}

void PlotPage::onLiveUpdate() {
    bool anyAlive = false;
    bool anyGrew = false;

    // Phase 1: fetch latest data from all live sources
    for (auto &src : m_liveSources) {
        if (src.roi.isNull()) continue;
        bool alive = src.roi->isProcessing();
        if (alive) anyAlive = true;

        SpeedData data = src.roi->getResult();
        if (data.size() > src.lastSize) {
            anyGrew = true;
            src.lastSize = data.size();
        }
        if (data.size() > 0)
            src.fullData = std::move(data);
    }

    // Phase 2: determine display limit (min max-distance across active live sources)
    int activeLiveCount = 0;
    double minMaxDist = 1e18;
    for (auto &src : m_liveSources) {
        if (src.fullData.distance.empty()) continue;
        activeLiveCount++;
        double maxDist = src.fullData.distance.back();
        if (maxDist < minMaxDist) minMaxDist = maxDist;
    }

    // Phase 3: display data clipped to minMaxDist (sync multiple curves)
    bool needSync = (activeLiveCount >= 2);
    for (auto &src : m_liveSources) {
        if (src.fullData.size() == 0) continue;
        if (src.analyzerIndex >= static_cast<int>(m_analyzers.size())) continue;

        if (needSync && minMaxDist < 1e17) {
            // Find how many points to display (up to minMaxDist)
            SpeedData clipped;
            clipped.name = src.fullData.name;
            for (int i = 0; i < src.fullData.size(); ++i) {
                if (src.fullData.distance[i] > minMaxDist) break;
                clipped.frame.push_back(src.fullData.frame[i]);
                clipped.speed.push_back(src.fullData.speed[i]);
                clipped.distance.push_back(src.fullData.distance[i]);
                if (i < static_cast<int>(src.fullData.time_s.size()))
                    clipped.time_s.push_back(src.fullData.time_s[i]);
                if (i < static_cast<int>(src.fullData.accel.size()))
                    clipped.accel.push_back(src.fullData.accel[i]);
            }
            m_analyzers[src.analyzerIndex]->replaceData(clipped);
        } else {
            m_analyzers[src.analyzerIndex]->replaceData(src.fullData);
        }
    }

    m_plot->refreshGraphData();
    m_plot->rescaleAxes();
    m_plot->replot();

    // Phase 4: adaptive interval
    if (anyGrew) {
        m_liveInterval = 100;
    } else {
        m_liveInterval = std::min(m_liveInterval + 50, 500);
    }

    if (!anyAlive) {
        // All done — show full data for all sources
        for (auto &src : m_liveSources) {
            if (src.fullData.size() > 0 && src.analyzerIndex < static_cast<int>(m_analyzers.size()))
                m_analyzers[src.analyzerIndex]->replaceData(src.fullData);
        }
        m_plot->refreshGraphData();
        m_plot->rescaleAxes();
        m_plot->replot();
        m_liveTimer->stop();
    } else {
        m_liveTimer->setInterval(m_liveInterval);
    }
}
