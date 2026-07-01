#include "PlotWidget.h"
#include <QMouseEvent>
#include <cmath>

static const QColor COLORS[] = {
    QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#d62728"), QColor("#9467bd")
};

PlotWidget::PlotWidget(QWidget *parent) : QCustomPlot(parent) {
    xAxis->setLabel("Distance (m)");
    yAxis->setLabel("Speed (km/h)");

    // disable default drag/zoom — we handle it manually
    setInteraction(QCP::iRangeDrag, false);
    setInteraction(QCP::iRangeZoom, false);

    m_hoverLabel = new QCPItemText(this);
    m_hoverLabel->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
    m_hoverLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    m_hoverLabel->position->setCoords(0.01, 0.98);
    m_hoverLabel->setFont(QFont("sans", 9));
    m_hoverLabel->setColor(Qt::white);
    m_hoverLabel->setPadding(QMargins(4, 4, 4, 4));
    m_hoverLabel->setBrush(QBrush(QColor(79, 79, 79, 153)));
    m_hoverLabel->setVisible(false);

    m_selRect = new QCPItemRect(this);
    m_selRect->setPen(QPen(QColor(100, 100, 255, 180)));
    m_selRect->setBrush(QBrush(QColor(100, 100, 255, 40)));
    m_selRect->setVisible(false);

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &PlotWidget::onAnimationTick);

    setMouseTracking(true);
}

void PlotWidget::addAnalyzer(SDAnalyzer *analyzer, const QColor &color) {
    m_analyzers.push_back(analyzer);

    auto *graph = addGraph();
    graph->setName(QString::fromStdString(analyzer->name()));
    graph->setPen(QPen(color, 2));
    graph->setSelectable(QCP::stWhole);
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
    legend->setVisible(true);
    replot();
}

void PlotWidget::setSelectedIndex(int idx) {
    m_selectedIndex = idx;
    for (size_t i = 0; i < m_graphs.size(); ++i) {
        auto pen = m_graphs[i]->pen();
        pen.setWidth(static_cast<int>(i) == m_selectedIndex ? 4 : 2);
        m_graphs[i]->setPen(pen);
    }
    replot();
}

void PlotWidget::registerHoverCallback(int index, HoverCallback cb) {
    while (static_cast<int>(m_hoverCallbacks.size()) <= index)
        m_hoverCallbacks.push_back(nullptr);
    m_hoverCallbacks[index] = std::move(cb);
}

void PlotWidget::refreshGraphData() {
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
        auto &d = m_analyzers[i]->data();
        QVector<double> xData(d.distance.begin(), d.distance.end());
        QVector<double> yData(d.speed.begin(), d.speed.end());
        m_graphs[i]->setData(xData, yData);
    }
    replot();
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

// ---- Mouse: Ctrl+drag = pan, direct drag = selection, click = select curve ----

void PlotWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ControlModifier) {
            // ctrl+drag → pan
            setInteraction(QCP::iRangeDrag, true);
            QCustomPlot::mousePressEvent(event);
        } else {
            // start rectangle selection
            m_selecting = true;
            m_selStartX = xAxis->pixelToCoord(event->pos().x());
            m_selRect->topLeft->setCoords(m_selStartX, yAxis->range().upper);
            m_selRect->bottomRight->setCoords(m_selStartX, yAxis->range().lower);
            m_selRect->setVisible(true);

            // check if clicking on a graph to select it
            double dist = xAxis->pixelToCoord(event->pos().x());
            double minDist = 1e18;
            int bestIdx = -1;
            for (size_t i = 0; i < m_analyzers.size(); ++i) {
                double spd = m_analyzers[i]->getSpeed(dist);
                double yPix = yAxis->coordToPixel(spd);
                double d = std::abs(yPix - event->pos().y());
                if (d < minDist) { minDist = d; bestIdx = static_cast<int>(i); }
            }
            if (minDist < 20.0) {
                setSelectedIndex(bestIdx);
            }
        }
    }
}

void PlotWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setInteraction(QCP::iRangeDrag, false);

        if (m_selecting) {
            m_selecting = false;
            m_selRect->setVisible(false);
            double x2 = xAxis->pixelToCoord(event->pos().x());
            double x1 = m_selStartX;
            if (x1 > x2) std::swap(x1, x2);
            if (x2 - x1 > 5.0) {
                handleSelection(x1, x2);
            }
            replot();
        }
    }
    QCustomPlot::mouseReleaseEvent(event);
}

void PlotWidget::mouseMoveEvent(QMouseEvent *event) {
    QCustomPlot::mouseMoveEvent(event);

    if (m_selecting) {
        double curX = xAxis->pixelToCoord(event->pos().x());
        double x1 = std::min(m_selStartX, curX), x2 = std::max(m_selStartX, curX);
        m_selRect->topLeft->setCoords(x1, yAxis->range().upper);
        m_selRect->bottomRight->setCoords(x2, yAxis->range().lower);
        replot(QCustomPlot::rpQueuedReplot);
        return;
    }

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
    // ctrl+scroll = zoom
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = (event->angleDelta().y() > 0) ? 0.85 : 1.18;
        double cx = xAxis->pixelToCoord(event->position().x());
        double cy = yAxis->pixelToCoord(event->position().y());
        auto xr = xAxis->range(), yr = yAxis->range();
        xAxis->setRange(cx - (cx - xr.lower) * factor, cx + (xr.upper - cx) * factor);
        yAxis->setRange(cy - (cy - yr.lower) * factor, cy + (yr.upper - cy) * factor);
        replot();
    }
}

// ---- Selection → time difference ----

double PlotWidget::computeSegmentTime(SDAnalyzer *a, double x1, double x2) {
    auto &d = a->data();
    double totalTime = 0;
    for (int i = 0; i < d.size() - 1; ++i) {
        if (d.distance[i] < x1 || d.distance[i] > x2) continue;
        if (d.distance[i + 1] > x2) break;
        double v0 = d.speed[i] * 1000.0 / 3600.0;
        double v1 = d.speed[i + 1] * 1000.0 / 3600.0;
        if (v0 <= 0 && v1 <= 0) continue;
        double vavg = (v0 + v1) / 2.0;
        if (vavg <= 0) continue;
        double dx = d.distance[i + 1] - d.distance[i];
        totalTime += dx / vavg;
    }
    return totalTime;
}

void PlotWidget::handleSelection(double x1, double x2) {
    if (m_analyzers.size() < 2) return;

    // remove old delta text near this region
    double midX = (x1 + x2) / 2.0;
    for (auto it = m_deltaTexts.begin(); it != m_deltaTexts.end();) {
        double tx = (*it)->position->coords().x();
        if (std::abs(tx - midX) < 500) {
            removeItem(*it);
            it = m_deltaTexts.erase(it);
        } else {
            ++it;
        }
    }

    double t1 = computeSegmentTime(m_analyzers[0], x1, x2);
    double t2 = computeSegmentTime(m_analyzers[1], x1, x2);
    double dt = t1 - t2;

    auto *txt = new QCPItemText(this);
    txt->position->setCoords(midX, yAxis->range().upper * 0.9);
    txt->setText(QString::fromUtf8("Δt = %1 s").arg(dt, 0, 'f', 3));
    txt->setFont(QFont("sans", 10));
    txt->setColor(QColor("#800080"));
    txt->setPadding(QMargins(4, 2, 4, 2));
    txt->setBrush(QBrush(QColor(255, 255, 255, 153)));
    m_deltaTexts.push_back(txt);
    replot();
}

// ---- Playing animation ----

void PlotWidget::startAnimation() {
    if (m_analyzers.empty()) return;
    m_animating = true;
    // use slowest fps among analyzers (approximate)
    m_animTimer->start(33); // ~30fps
}

void PlotWidget::stopAnimation() {
    m_animating = false;
    m_animTimer->stop();
}

void PlotWidget::onAnimationTick() {
    for (auto *a : m_analyzers)
        a->incCurrentIndex();
    updatePlot();

    // invoke hover callbacks to sync video
    for (size_t i = 0; i < m_analyzers.size(); ++i) {
        if (i < m_hoverCallbacks.size() && m_hoverCallbacks[i])
            m_hoverCallbacks[i](m_analyzers[i], static_cast<int>(i));
    }
}
