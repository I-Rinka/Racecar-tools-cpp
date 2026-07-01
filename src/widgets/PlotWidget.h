#pragma once
#include <QWidget>
#include <QTimer>
#include "qcustomplot.h"
#include "core/SDAnalyzer.h"
#include <vector>
#include <functional>

class PlotWidget : public QCustomPlot {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void addAnalyzer(SDAnalyzer *analyzer, const QColor &color);
    void updatePlot();
    void refreshGraphData();

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    using HoverCallback = std::function<void(SDAnalyzer *, int)>;
    void registerHoverCallback(int index, HoverCallback cb);

    std::vector<SDAnalyzer *> &analyzers() { return m_analyzers; }
    void setCtrlPressed(bool pressed) { m_ctrlPressed = pressed; }

    // playing animation
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const { return m_animating; }

signals:
    void distanceHovered(double distance);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onAnimationTick();

private:
    void handleSelection(double x1, double x2);
    double computeSegmentTime(SDAnalyzer *a, double x1, double x2);

    std::vector<SDAnalyzer *> m_analyzers;
    std::vector<QCPGraph *> m_graphs;
    std::vector<QCPGraph *> m_pointGraphs;
    std::vector<HoverCallback> m_hoverCallbacks;
    QCPItemText *m_hoverLabel = nullptr;
    int m_selectedIndex = -1;
    bool m_ctrlPressed = false;

    // selection rectangle
    bool m_selecting = false;
    double m_selStartX = 0;
    QCPItemRect *m_selRect = nullptr;
    std::vector<QCPItemText *> m_deltaTexts;

    // playing animation
    bool m_animating = false;
    QTimer *m_animTimer;
};
