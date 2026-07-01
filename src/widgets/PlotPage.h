#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QKeyEvent>
#include <vector>
#include "PlotWidget.h"
#include "VideoPlayer.h"
#include "core/SDAnalyzer.h"

class PlotPage : public QWidget {
    Q_OBJECT
public:
    explicit PlotPage(QWidget *parent = nullptr);

    void addInstance(const QString &csvPath, const QString &videoPath);
    void addInstanceByData(const QString &name, const SpeedData &data, const QString &videoPath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void refreshVideoLayout();
    void registerVideoHover(int index);

    PlotWidget *m_plot;
    QWidget *m_videoContainer;
    QGridLayout *m_videoLayout;
    std::vector<VideoPlayer *> m_videos;
    std::vector<SDAnalyzer *> m_analyzers;
    std::vector<double> m_videoFps;
    bool m_playing = false;
    QString m_pendingCsv;
};
