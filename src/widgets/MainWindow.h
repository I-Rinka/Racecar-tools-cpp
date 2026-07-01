#pragma once
#include <QMainWindow>
#include <QTabWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addPlotTab();
    void addAnalyzeTab();

private:
    void createMenus();
    void onTabCloseRequested(int index);

    QTabWidget *m_tabs;
};
