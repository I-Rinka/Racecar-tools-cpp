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
    int idx = m_tabs->addTab(page, tr("视频解析"));
    m_tabs->setCurrentIndex(idx);
}

void MainWindow::onTabCloseRequested(int index) {
    auto *w = m_tabs->widget(index);
    m_tabs->removeTab(index);
    w->deleteLater();
}
