#include "PlotWidget.h"
#include <QMouseEvent>
#include <cmath>

static const QColor COLORS[] = {
    QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#d62728"), QColor("#9467bd")
};

PlotWidget::PlotWidget(QWidget *parent) : QCustomPlot(parent) {
    xAxis->setLabel("Distance (m)");
    yAxis->setLabel("Speed (km/h)");
    setInteraction(QCP::iRangeDrag, true);
    setInteraction(QCP::iRangeZoom, true);

    m_hoverLabel = new QCPItemText(this);
    m_hoverLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_hoverLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    m_hoverLabel->position->setCoords(0.01, 0.98);
    m_hoverLabel->setFont(QFont("sans", 9));
    m_hoverLabel->setColor(Qt::white);
    m_hoverLabel->setPadding(QMargins(4, 4, 4, 4));
    m_hoverLabel->setBrush(QBrush(QColor(79, 79, 79, 153)));
    m_hoverLabel->setVisible(false);

    setMouseTracking(true);
}

void PlotWidget::addAnalyzer(SDAnalyzer *analyzer, const QColor &color) {
    m_analyzers.push_back(analyzer);

    auto *graph = addGraph();
    graph->setName(QString::fromStdString(analyzer->name()));
    graph->setPen(QPen(color, 2));
    auto &d = analyzer->data();
    QVector<double> xData(d.distance.begin(), d.distance.end());
    QVector<double> yData(d.speed.begin(), d.speed.end());
    graph->setData(xData, yData);
    m_graphs.push_back(graph);

    auto *pointGraph = addGraph();
    pointGraph->setLineStyle(QCPGraph::lsNone);
    pointGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color, Qt::white, 8));
    m_pointGraphs.push_back(pointGraph);

    m_hoverCallbacks.push_back(nullptr);

    rescaleAxes();
    replot();
}

void PlotWidget::registerHoverCallback(int index, HoverCallback cb) {
    if (index >= 0 && index < static_cast<int>(m_hoverCallbacks.size()))
        m_hoverCallbacks[index] = std::move(cb);
}

void PlotWidget::updatePlot() {
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
        auto *a = m_analyzers[i];
        auto &d = a->data();
        int ci = a->currentIndex();
        if (ci >= 0 && ci < d.size()) {
            m_pointGraphs[i]->setData({d.distance[ci]}, {d.speed[ci]});
        }
    }
    replot(QCustomPlot::rpQueuedReplot);
}

void PlotWidget::mouseMoveEvent(QMouseEvent *event) {
    QCustomPlot::mouseMoveEvent(event);

    double dist = xAxis->pixelToCoord(event->pos().x());

    QString text = QString("distance: %1m").arg(dist, 0, 'f', 2);
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
        auto *a = m_analyzers[i];
        a->setCurrentIndexByDistance(dist);
        double speed = a->getSpeed(dist);
        double accel = a->getAccel() / 9.8;
        text += QString("\n V%1: %2 km/h, a: %3g")
                    .arg(i).arg(speed, 0, 'f', 2).arg(accel, 0, 'f', 2);

        if (i < m_hoverCallbacks.size() && m_hoverCallbacks[i])
            m_hoverCallbacks[i](a, static_cast<int>(i));
    }

    m_hoverLabel->setText(text);
    m_hoverLabel->setVisible(true);
    updatePlot();
}

void PlotWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        QCustomPlot::wheelEvent(event);
    }
}
