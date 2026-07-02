#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QKeyEvent>
#include <QTimer>
#include <QPointer>
#include <QStackedWidget>
#include <vector>
#include "PlotWidget.h"
#include "VideoPlayer.h"
#include "DataEditor.h"
#include "core/SDAnalyzer.h"

class ROISelector;

class PlotPage : public QWidget {
    Q_OBJECT
public:
    explicit PlotPage(QWidget *parent = nullptr);

    void addInstance(const QString &csvPath, const QString &videoPath);
    void addInstanceByData(const QString &name, const SpeedData &data, const QString &videoPath);
    void addLiveInstance(ROISelector *roi);

signals:
    void roiTabDropped(int tabIndex);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onSyncTick();
    void onLiveUpdate();
    void onEditCurve();
    void onEditorClosed();

private:
    void refreshVideoLayout();
    void registerVideoHover(int index);

    PlotWidget *m_plot;
    QWidget *m_videoContainer;
    QHBoxLayout *m_videoLayout;
    QPushButton *m_editBtn;
    std::vector<VideoPlayer *> m_videos;
    std::vector<SDAnalyzer *> m_analyzers;
    std::vector<double> m_videoFps;
    bool m_playing = false;
    QString m_pendingCsv;
    QTimer *m_syncTimer;

    // Data editor mode
    QWidget *m_editorContainer = nullptr;
    DataEditor *m_editor = nullptr;
    VideoPlayer *m_editorVideo = nullptr;
    int m_editingIndex = -1;

    struct LiveSource {
        QPointer<ROISelector> roi;
        int analyzerIndex;
        SpeedData fullData;
        int lastSize = 0;
    };
    std::vector<LiveSource> m_liveSources;
    QTimer *m_liveTimer = nullptr;
    int m_liveInterval = 100;
};
