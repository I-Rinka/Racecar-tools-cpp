#include "MainWindow.h"
#include "PlotPage.h"
#include "ROISelector.h"
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("圈速工具"));

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    setCentralWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    m_tabs->tabBar()->installEventFilter(this);

    addPlotTab();
    addAnalyzeTab();
    m_tabs->setCurrentIndex(1);
    createMenus();
}

void MainWindow::createMenus() {
    auto *menu = menuBar()->addMenu(tr("文件"));
    menu->addAction(tr("新圈速分析"), this, &MainWindow::addPlotTab);
    menu->addAction(tr("新视频解析"), this, &MainWindow::addAnalyzeTab);
    menu->addSeparator();
    menu->addAction(tr("退出"), qApp, &QApplication::quit);
}

void MainWindow::addPlotTab() {
    auto *page = new PlotPage(this);
    connect(page, &PlotPage::roiTabDropped, this, [this, page](int tabIdx) {
        auto *roi = qobject_cast<ROISelector *>(m_tabs->widget(tabIdx));
        if (roi) page->addLiveInstance(roi);
    });
    int idx = m_tabs->addTab(page, tr("圈速分析"));
    m_tabs->setCurrentIndex(idx);
}

void MainWindow::addAnalyzeTab() {
    auto *page = new ROISelector(this);
    connect(page, &ROISelector::processingFinished,
            this, &MainWindow::onAnalysisFinished);
    int idx = m_tabs->addTab(page, tr("视频解析"));
    m_tabs->setCurrentIndex(idx);
}

void MainWindow::onAnalysisFinished(const QString &name, const SpeedData &data,
                                     const QString &videoPath) {
    PlotPage *plotPage = nullptr;
    for (int i = 0; i < m_tabs->count(); ++i) {
        plotPage = qobject_cast<PlotPage *>(m_tabs->widget(i));
        if (plotPage) {
            m_tabs->setCurrentIndex(i);
            break;
        }
    }
    if (!plotPage) {
        plotPage = new PlotPage(this);
        int idx = m_tabs->addTab(plotPage, tr("圈速分析"));
        m_tabs->setCurrentIndex(idx);
    }
    plotPage->addInstanceByData(name, data, videoPath);
}

void MainWindow::onTabCloseRequested(int index) {
    auto *w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    w->deleteLater();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_tabs->tabBar()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton)
                m_dragStartPos = me->pos();
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (!(me->buttons() & Qt::LeftButton))
                return QMainWindow::eventFilter(obj, event);
            if ((me->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
                return QMainWindow::eventFilter(obj, event);

            int tabIdx = m_tabs->tabBar()->tabAt(m_dragStartPos);
            if (tabIdx >= 0 && qobject_cast<ROISelector *>(m_tabs->widget(tabIdx))) {
                auto *drag = new QDrag(this);
                auto *mime = new QMimeData;
                mime->setData("application/x-racecar-tab-index",
                              QByteArray::number(tabIdx));
                drag->setMimeData(mime);
                drag->exec(Qt::CopyAction);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
