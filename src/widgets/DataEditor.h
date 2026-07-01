#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <vector>
#include <functional>
#include "core/SDAnalyzer.h"

class DataEditor : public QWidget {
    Q_OBJECT
public:
    DataEditor(SpeedData &data, const std::vector<int> &indices,
               std::function<void(const SpeedData &)> saveCallback,
               QWidget *parent = nullptr);

    void registerHoverCallback(std::function<void(int)> cb);

private slots:
    void onCellChanged(int row, int col);
    void onCellClicked(int row, int col);
    void findNext();
    void findOutstanding();
    void fillSelected();
    void saveAndClose();

private:
    SpeedData &m_data;
    std::vector<int> m_indices;
    std::function<void(const SpeedData &)> m_saveCallback;
    std::function<void(int)> m_hoverCallback;
    QTableWidget *m_table;
    QLineEdit *m_searchInput;
    QLabel *m_lastValLabel;
    QString m_lastEditedValue;
    int m_lastFindPos = -1;
    int m_lastFindPos2 = -1;
    bool m_updating = false;
};
