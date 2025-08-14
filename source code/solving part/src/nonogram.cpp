#include "nonogram.h"
#include <fstream>
#include <sstream>
#include <string>

void nonogram::free2Darray(int **temp, int size)
{
    for (int i = 0; i < size; ++i)
        delete[] temp[i];
    delete[] temp;
}

nonogram::nonogram(QObject *parent) : QThread(parent)
{
    m_isCanceled = false;
    m_isPaused = false;
}

nonogram::~nonogram()
{
}

void nonogram::solvingPuzzle(vector<vector<int>> &rowList, vector<vector<int>> &colList)
{
    nonogramSetInput(rowList, colList);
    start(LowPriority);
}

void nonogram::nonogramSetInput(vector<vector<int>> &rowList, vector<vector<int>> &colList)
{
    groupListInput(rowList, colList);
}
void nonogram::nonogramSetInput()
{
    // typingInput();
    fileInput();
}

void nonogram::groupListInput(vector<vector<int>> &rowList, vector<vector<int>> &colList)
{
    allFinished = false;
    row = rowList.size();
    col = colList.size();

    rowList_blockGroup = new int *[row];
    rowList_blockGroup_size = new int[row];
    rowList_totalBlock = new int[row];
    colList_blockGroup = new int *[col];
    colList_blockGroup_size = new int[col];
    colList_totalBlock = new int[col];

    rowPossibleFilling = new int[row];
    fill_n(rowPossibleFilling, row, 2);
    colPossibleFilling = new int[col];
    fill_n(colPossibleFilling, col, 2);

    for (int i = 0; i < row; ++i)
    {
        int blockNum = 0;
        int size = rowList[i].size();
        if (size == 1 && rowList[i][0] == 0)
            rowList_blockGroup_size[i] = 0;
        else
            rowList_blockGroup_size[i] = size;
        rowList_blockGroup[i] = new int[size];

        for (int j = 0; j < size; ++j)
        {
            blockNum += rowList[i][j];
            rowList_blockGroup[i][j] = rowList[i][j];
        }
        rowList_totalBlock[i] = blockNum;
    }
    for (int i = 0; i < col; ++i)
    {
        int blockNum = 0;
        int size = colList[i].size();
        if (size == 1 && colList[i][0] == 0)
            colList_blockGroup_size[i] = 0;
        else
            colList_blockGroup_size[i] = size;
        colList_blockGroup[i] = new int[size];

        for (int j = 0; j < size; ++j)
        {
            blockNum += colList[i][j];
            colList_blockGroup[i][j] = colList[i][j];
        }
        colList_totalBlock[i] = blockNum;
    }

    isRowFinished = new int[(row >> 5) + (((row & 0b11111) == 0) ? 0 : 1)];
    isColFinished = new int[(col >> 5) + (((col & 0b11111) == 0) ? 0 : 1)];
    int row_temp;
    int idx = 0;
    for (row_temp = row; row_temp >= 32; row_temp -= 32)
    {
        isRowFinished[idx++] = 0;
    }
    if (row_temp > 0)
    {
        isRowFinished[idx++] = 0;
        isRowFinishedMask = (1 << row_temp) - 1;
    }
    int col_temp;
    idx = 0;
    for (col_temp = col; col_temp >= 32; col_temp -= 32)
    {
        isColFinished[idx++] = 0;
    }
    if (col_temp > 0)
    {
        isColFinished[idx++] = 0;
        isColFinishedMask = (1 << col_temp) - 1;
    }

    nonogramGraph = new int *[row];
    for (int i = 0; i < row; ++i)
    {
        nonogramGraph[i] = new int[col];
        for (int j = 0; j < col; ++j)
            nonogramGraph[i][j] = -1;
    }
}

void nonogram::fileInput()
{
    allFinished = false;
    ifstream file("test.txt");

    // String to store each line of the file.
    string str;

    if (file.is_open())
    {
        // Read each line from the file and store it in the
        // 'line' variable.
        getline(file, str);
        stringstream stringStream(str);
        stringStream >> row >> col;

        rowList_blockGroup = new int *[row];
        rowList_blockGroup_size = new int[row];
        rowList_totalBlock = new int[row];
        colList_blockGroup = new int *[col];
        colList_blockGroup_size = new int[col];
        colList_totalBlock = new int[col];

        rowPossibleFilling = new int[row];
        fill_n(rowPossibleFilling, row, 2);
        colPossibleFilling = new int[col];
        fill_n(colPossibleFilling, col, 2);

        int idx = 0;
        int *temp = new int[max(row, col)];
        while (getline(file, str))
        {
            int val = 0;
            stringstream stringStream(str);
            if (idx < row)
            {
                int blockNum = 0;
                int size = 0;
                while (stringStream >> val)
                {
                    blockNum += val;
                    temp[size] = val;
                    ++size;
                }
                if (size == 1 && val == 0)
                    rowList_blockGroup_size[idx] = 0;
                else
                    rowList_blockGroup_size[idx] = size;
                rowList_blockGroup[idx] = new int[size];
                memcpy(rowList_blockGroup[idx], temp, size * sizeof(int));
                rowList_totalBlock[idx] = blockNum;
            }
            else
            {
                int blockNum = 0;
                int size = 0;
                while (stringStream >> val)
                {
                    blockNum += val;
                    temp[size] = val;
                    ++size;
                }
                if (size == 1 && val == 0)
                    colList_blockGroup_size[idx - row] = 0;
                else
                    colList_blockGroup_size[idx - row] = size;
                colList_blockGroup[idx - row] = new int[size];
                memcpy(colList_blockGroup[idx - row], temp, size * sizeof(int));
                colList_totalBlock[idx - row] = blockNum;
            }
            ++idx;
        }
        delete[] temp;

        isRowFinished = new int[(row >> 5) + (((row & 0b11111) == 0) ? 0 : 1)];
        isColFinished = new int[(col >> 5) + (((col & 0b11111) == 0) ? 0 : 1)];
        int row_temp;
        idx = 0;
        for (row_temp = row; row_temp >= 32; row_temp -= 32)
        {
            isRowFinished[idx++] = 0;
        }
        if (row_temp > 0)
        {
            isRowFinished[idx++] = 0;
            isRowFinishedMask = (1 << row_temp) - 1;
        }
        int col_temp;
        idx = 0;
        for (col_temp = col; col_temp >= 32; col_temp -= 32)
        {
            isColFinished[idx++] = 0;
        }
        if (col_temp > 0)
        {
            isColFinished[idx++] = 0;
            isColFinishedMask = (1 << col_temp) - 1;
        }

        nonogramGraph = new int *[row];
        for (int i = 0; i < row; ++i)
        {
            nonogramGraph[i] = new int[col];
            for (int j = 0; j < col; ++j)
                nonogramGraph[i][j] = -1;
        }
        file.close();
    }
}

void nonogram::typingInput()
{
    allFinished = false;

    while (row <= 0 || col <= 0)
    {
        cout << "please enter the number of row and columns: ";
        cin >> row >> col;
    }
    cout << endl
         << "now enter the block groups for each row: " << endl;

    rowList_blockGroup = new int *[row];
    rowList_blockGroup_size = new int[row];
    rowList_totalBlock = new int[row];
    colList_blockGroup = new int *[col];
    colList_blockGroup_size = new int[col];
    colList_totalBlock = new int[col];

    rowPossibleFilling = new int[row];
    fill_n(rowPossibleFilling, row, 2);
    colPossibleFilling = new int[col];
    fill_n(colPossibleFilling, col, 2);

    int idx = 0;
    for (int i = 0; i < row; ++i)
    {
        int group = -1;
        while (group < 0)
        {
            cout << "how many number of block groups in row " << i << ":";
            cin >> group;
        }
        int blockNum = 0;

        rowList_blockGroup[idx] = new int[group];
        do
        {
            blockNum = 0;
            int val;
            cout << "Enter the val of each groups: " << endl;
            for (int j = 0; j < group; ++j)
            {
                cin >> val;
                rowList_blockGroup[idx][j] = val;
                blockNum += val;
            }
        } while (blockNum + group - 1 > col);
        rowList_blockGroup_size[idx] = group;
        rowList_totalBlock[idx] = blockNum;
        ++idx;
    }

    idx = 0;
    for (int i = 0; i < col; ++i)
    {
        int group = -1;
        while (group < 0)
        {
            cout << "how many number of block groups in col " << i << ":";
            cin >> group;
        }
        int blockNum = 0;

        colList_blockGroup[idx] = new int[group];
        do
        {
            blockNum = 0;
            int val;
            cout << "Enter the val of each groups: " << endl;
            for (int j = 0; j < group; ++j)
            {
                cin >> val;
                colList_blockGroup[idx][j] = val;
                blockNum += val;
            }
        } while (blockNum + group - 1 > col);
        colList_blockGroup_size[idx] = group;
        colList_totalBlock[idx] = blockNum;
        ++idx;
    }

    isRowFinished = new int[(row >> 5) + (((row & 0b11111) == 0) ? 0 : 1)];
    isColFinished = new int[(col >> 5) + (((col & 0b11111) == 0) ? 0 : 1)];
    int row_temp;
    idx = 0;
    for (row_temp = row; row_temp >= 32; row_temp -= 32)
    {
        isRowFinished[idx++] = 0;
    }
    if (row_temp > 0)
    {
        isRowFinished[idx++] = 0;
        isRowFinishedMask = (1 << row_temp) - 1;
    }
    int col_temp;
    idx = 0;
    for (col_temp = col; col_temp >= 32; col_temp -= 32)
    {
        isColFinished[idx++] = 0;
    }
    if (col_temp > 0)
    {
        isColFinished[idx++] = 0;
        isColFinishedMask = (1 << col_temp) - 1;
    }

    nonogramGraph = new int *[row];
    for (int i = 0; i < row; ++i)
    {
        nonogramGraph[i] = new int[col];
        for (int j = 0; j < col; ++j)
            nonogramGraph[i][j] = -1;
    }
}
