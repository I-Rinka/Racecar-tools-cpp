#pragma once
#include <QWidget>
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
    int selectedIndex() const { return m_selectedIndex; }

    using HoverCallback = std::function<void(SDAnalyzer *, int)>;
    void registerHoverCallback(int index, HoverCallback cb);

    std::vector<SDAnalyzer *> &analyzers() { return m_analyzers; }

signals:
    void distanceHovered(double distance);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    std::vector<SDAnalyzer *> m_analyzers;
    std::vector<QCPGraph *> m_graphs;
    std::vector<QCPGraph *> m_pointGraphs;
    std::vector<HoverCallback> m_hoverCallbacks;
    QCPItemText *m_hoverLabel = nullptr;
    int m_selectedIndex = -1;
    bool m_ctrlPressed = false;
};
