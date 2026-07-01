#include "DataEditor.h"
#include <QMessageBox>
#include <QHeaderView>
#include <cmath>

DataEditor::DataEditor(SpeedData &data, const std::vector<int> &indices,
                       std::function<void(const SpeedData &)> saveCallback,
                       QWidget *parent)
    : QWidget(parent), m_data(data), m_indices(indices),
      m_saveCallback(std::move(saveCallback)) {
    setWindowTitle(tr("编辑选中数据"));
    resize(720, 600);

    auto *layout = new QVBoxLayout(this);

    auto *topLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("输入要查找的内容..."));
    auto *searchBtn = new QPushButton(tr("查找下一个"), this);
    auto *searchBtn2 = new QPushButton(tr("查找突出值"), this);
    connect(searchBtn, &QPushButton::clicked, this, &DataEditor::findNext);
    connect(searchBtn2, &QPushButton::clicked, this, &DataEditor::findOutstanding);

    topLayout->addWidget(new QLabel(tr("查找："), this));
    topLayout->addWidget(m_searchInput);
    topLayout->addWidget(searchBtn);
    topLayout->addWidget(searchBtn2);

    m_lastValLabel = new QLabel(tr("上次修改值：<无>"), this);
    topLayout->addWidget(m_lastValLabel);

    auto *fillBtn = new QPushButton(tr("填充选中"), this);
    connect(fillBtn, &QPushButton::clicked, this, &DataEditor::fillSelected);
    topLayout->addWidget(fillBtn);

    layout->addLayout(topLayout);

    m_table = new QTableWidget(static_cast<int>(m_indices.size()), 2, this);
    m_table->setHorizontalHeaderLabels({tr("distance"), tr("speed")});
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);

    for (int row = 0; row < static_cast<int>(m_indices.size()); ++row) {
        int idx = m_indices[row];
        m_table->setItem(row, 0, new QTableWidgetItem(
            QString::number(m_data.distance[idx], 'f', 2)));
        m_table->setItem(row, 1, new QTableWidgetItem(
            QString::number(m_data.speed[idx], 'f', 2)));
    }
    connect(m_table, &QTableWidget::cellChanged, this, &DataEditor::onCellChanged);
    connect(m_table, &QTableWidget::cellClicked, this, &DataEditor::onCellClicked);
    layout->addWidget(m_table);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto *saveBtn = new QPushButton(tr("保存"), this);
    connect(saveBtn, &QPushButton::clicked, this, &DataEditor::saveAndClose);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);
}

void DataEditor::registerHoverCallback(std::function<void(int)> cb) {
    m_hoverCallback = std::move(cb);
}

void DataEditor::onCellClicked(int row, int) {
    if (m_hoverCallback && row >= 0 && row < static_cast<int>(m_indices.size()))
        m_hoverCallback(m_indices[row]);
}

void DataEditor::onCellChanged(int row, int col) {
    if (m_updating) return;
    auto *item = m_table->item(row, col);
    if (!item) return;

    int idx = m_indices[row];
    bool ok;
    double val = item->text().toDouble(&ok);
    if (ok) {
        if (col == 0) m_data.distance[idx] = val;
        else m_data.speed[idx] = val;
    }

    m_lastEditedValue = item->text();
    m_lastValLabel->setText(tr("上次修改值：%1").arg(m_lastEditedValue));
}

void DataEditor::findNext() {
    QString text = m_searchInput->text().trimmed();
    if (text.isEmpty()) return;
    int rows = m_table->rowCount();
    int cols = m_table->columnCount();
    int total = rows * cols;
    int start = (m_lastFindPos + 1) % total;
    for (int step = 0; step < total; ++step) {
        int linear = (start + step) % total;
        int r = linear / cols, c = linear % cols;
        auto *item = m_table->item(r, c);
        if (item && item->text() == text) {
            m_table->setCurrentCell(r, c);
            m_table->scrollToItem(item);
            m_lastFindPos = linear;
            return;
        }
    }
    QMessageBox::information(this, tr("查找结果"), tr("未找到"));
    m_lastFindPos = -1;
}

void DataEditor::findOutstanding() {
    int rows = m_table->rowCount();
    int start = m_lastFindPos2 + 1;
    for (int i = start; i < rows; ++i) {
        auto *item = m_table->item(i, 1);
        if (!item) continue;
        bool ok;
        double v = item->text().toDouble(&ok);
        if (!ok || v <= 40.0 || v >= 500.0) {
            m_table->setCurrentCell(i, 1);
            m_table->scrollToItem(item);
            m_lastFindPos2 = i;
            return;
        }
    }
    QMessageBox::information(this, tr("查找结果"), tr("未找到突出值"));
    m_lastFindPos2 = -1;
}

void DataEditor::fillSelected() {
    if (m_lastEditedValue.isEmpty()) return;
    m_updating = true;
    for (auto &idx : m_table->selectionModel()->selectedIndexes()) {
        auto *item = m_table->item(idx.row(), idx.column());
        if (item) item->setText(m_lastEditedValue);
    }
    m_updating = false;
}

void DataEditor::saveAndClose() {
    for (int row = 0; row < static_cast<int>(m_indices.size()); ++row) {
        int idx = m_indices[row];
        auto *xItem = m_table->item(row, 0);
        auto *yItem = m_table->item(row, 1);
        if (xItem && yItem) {
            bool okX, okY;
            double x = xItem->text().toDouble(&okX);
            double y = yItem->text().toDouble(&okY);
            if (okX) m_data.distance[idx] = x;
            if (okY) m_data.speed[idx] = y;
        }
    }
    if (m_saveCallback) m_saveCallback(m_data);
    close();
}
