#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTabBar>
#include <QPoint>
#include "core/SDAnalyzer.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void addPlotTab();
    void addAnalyzeTab();
    void onAnalysisFinished(const QString &name, const SpeedData &data, const QString &videoPath);

private:
    void createMenus();
    void onTabCloseRequested(int index);

    QTabWidget *m_tabs;
    QPoint m_dragStartPos;
};
