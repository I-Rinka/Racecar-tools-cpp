#include "MainWindow.h"
#include "PlotPage.h"
#include "ROISelector.h"
#include <QMenuBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("圈速工具"));

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    setCentralWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

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
