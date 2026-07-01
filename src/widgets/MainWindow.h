#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include "core/SDAnalyzer.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addPlotTab();
    void addAnalyzeTab();
    void onAnalysisFinished(const QString &name, const SpeedData &data, const QString &videoPath);

private:
    void createMenus();
    void onTabCloseRequested(int index);

    QTabWidget *m_tabs;
};
