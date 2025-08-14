#include "nonogramboard.h"

void NonogramBoard::tableAddRow()
{
    if (fillingTable->rowCount() <= 1.5 * rowNum)
    {
        double cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());
        fillingTable->insertRow(0);
        fillingReadOnly(0, 0, 0, fillingTable->columnCount() - colNum - 1);
        tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount() * cellSize) / static_cast<double>(fillingTable->columnCount() * cellSize));
        tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));
        setTableMinMaxSize();
    }
}
void NonogramBoard::tableDelRow()
{
    if (fillingTable->rowCount() > rowNum + 1)
    {
        double cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());
        fillingTable->removeRow(0);
        tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount() * cellSize) / static_cast<double>(fillingTable->columnCount() * cellSize));
        tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));
        setTableMinMaxSize();
    }
}
void NonogramBoard::tableAddCol()
{
    if (fillingTable->columnCount() <= 1.5 * colNum)
    {
        double cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());
        fillingTable->insertColumn(0);
        fillingReadOnly(0, fillingTable->rowCount() - rowNum - 1, 0, 0);
        tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount() * cellSize) / static_cast<double>(fillingTable->columnCount() * cellSize));
        tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));
        setTableMinMaxSize();
    }
}
void NonogramBoard::tableDelCol()
{
    if (fillingTable->columnCount() > colNum + 1)
    {
        double cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());
        fillingTable->removeColumn(0);
        tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount() * cellSize) / static_cast<double>(fillingTable->columnCount() * cellSize));
        tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));
        setTableMinMaxSize();
    }
}

void NonogramBoard::fillingTableGenerateHelper(int rowFillNumber, int colFillNumber, vector<vector<int>> *rowList, vector<vector<int>> *colList)
{
    fillingTable->clear(); // Removes all items in the view.

    fillingTable->setRowCount(rowNum);
    fillingTable->setColumnCount(colNum);
    fillingTable->setVisible(true);

    fillingReadOnly(0, rowNum - 1, 0, colNum - 1);

    // default 2 groups num to fill for each row and col
    for (int i = 0; i < colFillNumber; ++i)
    {
        fillingTable->insertRow(0);
    }
    for (int i = 0; i < rowFillNumber; ++i)
    {
        fillingTable->insertColumn(0);
    }
    fillingReadOnly(0, colFillNumber - 1, 0, rowFillNumber - 1);

    setTableMinMaxSize();

    if (rowList != nullptr) // if it has input data
    {
        fillingTableFillInputDataHelper(*rowList, *colList, colFillNumber - 1, rowFillNumber - 1);
    }
}

void NonogramBoard::fillingTableFillInputDataHelper(vector<vector<int>> &rowList, vector<vector<int>> &colList, int tableUpperRowIdx, int tableUpperColIdx)
{
    // colList hint number fill in the area: col: tableUpperColIdx + 1 ~ end, row: 0 ~ tableUpperRowIdx
    // rowList hint number fill in the area: col: 0 ~ tableUpperColIdx, row: tableUpperRowIdx + 1 ~ end
    //  handle rowList hint numbers first
    int cur_row_idx = tableUpperRowIdx + 1, cur_col_idx = tableUpperColIdx;
    QString num_str;
    for (int i = 0; i < rowList.size(); i += 1)
    {
        cur_col_idx = tableUpperColIdx;
        for (int j = rowList[i].size() - 1; j >= 0; j -= 1)
        {
            QTableWidgetItem *item = new QTableWidgetItem();
            num_str = QString::number(rowList[i][j]);
            item->setText(num_str);
            fillingTable->setItem(cur_row_idx, cur_col_idx, item);
            cur_col_idx -= 1;
        }
        cur_row_idx += 1;
    }
    // handle colList hint numbers
    cur_row_idx = tableUpperRowIdx, cur_col_idx = tableUpperColIdx + 1;
    for (int i = 0; i < colList.size(); i += 1)
    {
        cur_row_idx = tableUpperRowIdx;
        for (int j = colList[i].size() - 1; j >= 0; j -= 1)
        {
            QTableWidgetItem *item = new QTableWidgetItem();
            num_str = QString::number(colList[i][j]);
            item->setText(num_str);
            fillingTable->setItem(cur_row_idx, cur_col_idx, item);
            cur_row_idx -= 1;
        }
        cur_col_idx += 1;
    }
}

void NonogramBoard::fillingReadOnly(int row1, int row2, int col1, int col2)
{
    for (int i = row1; i <= row2; ++i)
    {
        for (int j = col1; j <= col2; ++j)
        {
            if (fillingTable->item(i, j))
                delete fillingTable->item(i, j);
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsSelectable));
            item->setBackground(READONLY_COLOR);
            fillingTable->setItem(i, j, item);
        }
    }
}

void NonogramBoard::setTableMinMaxSize()
{
    fillingTable->setMinimumSize(fillingTable->columnCount() * CELL_SIZE_MIN, fillingTable->rowCount() * CELL_SIZE_MIN);
    fillingTable->setMaximumSize(fillingTable->columnCount() * CELL_SIZE_MAX, fillingTable->rowCount() * CELL_SIZE_MAX);
}

void NonogramBoard::QmessageReceive(QString title, QString text)
{
    QMessageBox::about(this, title, text);
}

void NonogramBoard::clearFinishedHandler(int outcome)
{
    delete gram;
    gram = nullptr;
    if (!modifyButton->isEnabled())
    {
        setButton->setEnabled(true);
    }
    stopButton->setEnabled(false);
    startButton->setEnabled(false);
    if (outcome == SUCCESS)
    {
        isCanvasSaved = false;
        stateChanged(STATE_GET_SOLVING_SUCCESS_OUTCOME);
    }
    else
    {
        stateChanged(STATE_GET_SOLVING_FAIL_OUTCOME);
    }
}