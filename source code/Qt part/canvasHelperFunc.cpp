#include "nonogramboard.h"
#include <QPainter>
#include <QPen>

// ref: https://forum.qt.io/topic/117497/how-to-move-a-draw-qpainter-to-qlabel/14

void NonogramBoard::canvasDrawTable()
{
    double width = fillingTable->width();
    double height = fillingTable->height();
    int totalRowNum = fillingTable->rowCount();
    int totalColNum = fillingTable->columnCount();
    // QRect wRect = fillingTable->rect();
    double cellSize = width / static_cast<double>(totalColNum);
    QPixmap pix(width, height); // give pixmap some size
    QPainter painter(&pix);     // assign painter to it. note the &
    // painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(UNSURE_COLOR);

    pix.fill(Qt::white); // looks like the default background color is black, so we use this line to make pixmap become white
    // painter.drawRect(wRect);

    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
        {
            painter.drawRect(i * cellSize, j * cellSize, cellSize, cellSize);
        }
    }

    for (int i = 0; i < totalColNum - colNum; ++i)
    {
        for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
        {
            if (fillingTable->item(j, i) == nullptr)
                continue;
            painter.drawText(i * cellSize, j * cellSize, cellSize, cellSize, Qt::AlignVCenter, fillingTable->item(j, i)->text());
        }
    }

    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        for (int j = 0; j < totalRowNum - rowNum; ++j)
        {
            if (fillingTable->item(j, i) == nullptr) // if the user didn't type anything at a cell, then there is no item at that place
                continue;
            painter.drawText(i * cellSize, j * cellSize, cellSize, cellSize, Qt::AlignBottom | Qt::AlignCenter, fillingTable->item(j, i)->text());
        }
    }

    canvas->setPixmap(pix);
}

int NonogramBoard::generateBlockEmptyList(vector<vector<int>> &rowList, vector<vector<int>> &colList)
{
    int totalRowNum = fillingTable->rowCount();
    int totalColNum = fillingTable->columnCount();
    bool convertSuccess = true;
    int idx = 0;
    int sum = 0;
    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        sum = 0;
        for (int j = 0; j < totalRowNum - rowNum; ++j)
        {
            if (fillingTable->item(j, i) == nullptr) // if the user didn't type anything at a cell, then there is no item at that place
                continue;
            if ((fillingTable->item(j, i)->text().isEmpty()))
            {
                delete fillingTable->item(j, i);
                continue;
            }
            int val = fillingTable->item(j, i)->text().toInt(&convertSuccess);
            if (!convertSuccess || val < 0)
                return GENERATE_LIST_FAIL_NOT_INT;
            if (!colList[idx].empty() && colList[idx].back() == 0 && val == 0)
                continue;
            colList[idx].push_back(val);
            sum += val;
        }
        if (colList[idx].empty())
            colList[idx].push_back(0);
        sum += colList[idx].size() - 1;
        if (sum > rowNum)
            return GENERATE_LIST_FAIL_EXCEED;
        ++idx;
    }
    idx = 0;
    sum = 0;
    for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
    {
        sum = 0;
        for (int i = 0; i < totalColNum - colNum; ++i)
        {
            if (fillingTable->item(j, i) == nullptr) // if the user didn't type anything at a cell, then there is no item at that place
                continue;
            if ((fillingTable->item(j, i)->text().isEmpty()))
            {
                delete fillingTable->item(j, i);
                continue;
            }
            int val = fillingTable->item(j, i)->text().toInt(&convertSuccess);
            if (!convertSuccess || val < 0)
                return GENERATE_LIST_FAIL_NOT_INT;
            if (!rowList[idx].empty() && rowList[idx].back() == 0 && val == 0)
                continue;
            rowList[idx].push_back(val);
            sum += val;
        }
        if (rowList[idx].empty())
            rowList[idx].push_back(0);
        sum += rowList[idx].size() - 1;
        if (sum > colNum)
            return GENERATE_LIST_FAIL_EXCEED;
        ++idx;
    }
    return GENERATE_LIST_SUCCESS;
}

void NonogramBoard::drawSpecificCell(int r, int c, int val)
{
    double width = width_during_solving;
    double height = height_during_solving;

    int totalRowNum = fillingTable->rowCount();
    int totalColNum = fillingTable->columnCount();
    // QRect wRect = fillingTable->rect();
    double cellSize = width / static_cast<double>(totalColNum);
    QPixmap pix = canvas->pixmap();
    QPainter painter(&pix);

    int table_c = totalColNum - colNum + c;
    int table_r = totalRowNum - rowNum + r;

    QTableWidgetItem *item = fillingTable->item(table_r, table_c);
    if (item)
        delete item;
    item = new QTableWidgetItem();

    if (val == EMPTY_VAL)
    {
        painter.setBrush(EMPTY_COLOR);
        item->setData(Qt::DisplayRole, EMPTY_VAL);
    }
    else
    {
        painter.setBrush(FILLING_COLOR);
        item->setData(Qt::DisplayRole, FILLING_VAL);
    }

    painter.drawRect((table_c)*cellSize, (table_r)*cellSize, cellSize, cellSize);
    canvas->setPixmap(pix);

    fillingTable->setItem(table_r, table_c, item);
}

void NonogramBoard::canvasRedraw()
{
    if (isInWorkingState()) // do not redraw the table when there is no outcome yet
        return;
    if (tableParentWidget == nullptr || fillingTable == nullptr)
        return;
    // use the width and heigh of aspectratio object to get the current width and height of canvas, and use them to calculate cellSize
    // cause when the window is enlarged or reduced, the actual size of canvas and fillingTable remain the same.
    double width = tableParentWidget->width();
    double height = tableParentWidget->height();
    if (width == 0)
        return;
    double aspect = height / width;
    double m_ratio = tableParentWidget->getRatio();
    if (aspect < m_ratio) // parent is too wide
    {
        width = height / m_ratio;
    }
    else // parent is too high
    {
        height = width * m_ratio;
    }

    int totalRowNum = fillingTable->rowCount();
    int totalColNum = fillingTable->columnCount();
    // QRect wRect = fillingTable->rect();
    if (totalRowNum == 0 || totalColNum == 0)
        return;
    double cellSize = width / static_cast<double>(totalColNum);
    QPixmap pix(width, height); // give pixmap some size
    QPainter painter(&pix);     // assign painter to it. note the &
    painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(UNSURE_COLOR);

    pix.fill(Qt::white); // looks like the default background color is black, so we use this line to make pixmap become white

    // draw grids
    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
        {
            painter.drawRect(i * cellSize, j * cellSize, cellSize, cellSize);
        }
    }

    // draw fill blocks
    QTableWidgetItem *item = nullptr;
    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
        {
            item = fillingTable->item(j, i);
            if (item == nullptr)
                continue;
            QVariant value = item->data(Qt::DisplayRole);
            if (value.canConvert<int>())
            {
                int fillColor = value.toInt();
                if (fillColor == EMPTY_VAL)
                {
                    painter.setBrush(EMPTY_COLOR);
                }
                else
                {
                    painter.setBrush(FILLING_COLOR);
                }
                painter.drawRect(i * cellSize, j * cellSize, cellSize, cellSize);
            }
        }
    }

    painter.setPen(FILLING_COLOR);
    // draw col hint numbers
    for (int i = 0; i < totalColNum - colNum; ++i)
    {
        for (int j = totalRowNum - rowNum; j < totalRowNum; ++j)
        {
            item = fillingTable->item(j, i);
            if (item == nullptr)
                continue;
            painter.drawText(i * cellSize, j * cellSize, cellSize, cellSize, Qt::AlignVCenter, item->text());
        }
    }

    // draw row hint numbers
    for (int i = totalColNum - colNum; i < totalColNum; ++i)
    {
        for (int j = 0; j < totalRowNum - rowNum; ++j)
        {
            item = fillingTable->item(j, i);
            if (item == nullptr) // if the user didn't type anything at a cell, then there is no item at that place
                continue;
            painter.drawText(i * cellSize, j * cellSize, cellSize, cellSize, Qt::AlignBottom | Qt::AlignCenter, item->text());
        }
    }
    canvas->setPixmap(pix);
}