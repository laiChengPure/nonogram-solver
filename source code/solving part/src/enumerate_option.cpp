#include "nonogram.h"

#include <time.h>

int idx = 0;
// this function is for enumeration step to find the definite place to fill 0 or 1, till we cannot find any clue from the block groups of row and column
//  then we will go to the next step: guessing
void nonogram::enumerateStep()
{
    int sameCnt = 0; // if sameCnt >= row + col, which means that all the rowws and columns do not change, so we can go to the next step
    // enumerate all the possible option and find the intersect,by doing this, we can fill definite value in the nonogram
    // till there is no change for all rows and cols, it can use sameCnt >= row + column to determine
    while (1)
    {
        for (int i = 0; i < row; ++i)
        {
            if (feasible == false || m_isCanceled)
                return;
            isPause();
            if (isRowOrColFinished(i, _ROW_CHOOSE))
            {
                setRowOrColFinished(i, _ROW_CHOOSE);
                ++sameCnt;
                continue;
            }
            isPause();
            // if return true, means something change
            if (intersectOption(i, _ROW_CHOOSE))
            {
                sameCnt = 0;
                printNonogram(i, _ROW_CHOOSE);
                if (isRowOrColFinished(i, _ROW_CHOOSE))
                    setRowOrColFinished(i, _ROW_CHOOSE);
            }
            else
                ++sameCnt;
            isPause();
            if (sameCnt >= row + col)
                break;
        }
        isPause();
        if (sameCnt >= row + col)
            break;
        for (int j = 0; j < col; ++j)
        {
            if (feasible == false || m_isCanceled)
                return;
            isPause();
            if (isRowOrColFinished(j, _COL_CHOOSE))
            {
                setRowOrColFinished(j, _COL_CHOOSE);
                ++sameCnt;
                continue;
            }
            isPause();
            // if return true, means something change
            if (intersectOption(j, _COL_CHOOSE))
            {
                sameCnt = 0;
                printNonogram(j, _COL_CHOOSE);
                if (isRowOrColFinished(j, _COL_CHOOSE))
                    setRowOrColFinished(j, _COL_CHOOSE);
            }
            else
                ++sameCnt;
            isPause();
            if (sameCnt >= row + col)
                break;
        }
        isPause();
        if (feasible == false || m_isCanceled)
            return;
        if (sameCnt >= row + col)
            break;
    }
    isPause();
    if (feasible == false || m_isCanceled)
        return;
    if (isAllRowColFinished())
        allFinished = true;
}

bool nonogram::intersectOption(int idx, int rowOrCol)
{
    int empty_num;
    int groupNum;
    if (rowOrCol == _ROW_CHOOSE)
    {
        empty_num = col - rowList_totalBlock[idx]; // total number of empty squares in this row idx
        groupNum = rowList_blockGroup_size[idx];   // total group number(for consecutive block square / group) in this row idx
    }
    else
    {
        empty_num = row - colList_totalBlock[idx]; // total number of empty squares in this col idx
        groupNum = colList_blockGroup_size[idx];   // total group number(for consecutive block square / group) in this col idx
    }
    isPause();
    // temp vector to record group(for consecutive empty square / group) in this row / col idx
    // since groupNum = number of group of block squares, so the number of group of empty squares is cellNum + 1
    int *emptyGroup = new int[groupNum + 1];

    int *blockSet = nullptr;
    int *emptySet = nullptr;
    if (rowOrCol == _ROW_CHOOSE)
    {
        blockSet = new int[col];
        emptySet = new int[col];
        for (int c = 0; c < col; ++c)
        {
            isPause();
            emptySet[c] = 1;
            blockSet[c] = 1;
        }
        for (int c = 0; c < col; ++c)
        {
            isPause();
            if (nonogramGraph[idx][c] == 1)
                emptySet[c] = 0;
            else if (nonogramGraph[idx][c] == 0)
                blockSet[c] = 0;
        }
    }
    else
    {
        blockSet = new int[row];
        emptySet = new int[row];
        for (int r = 0; r < row; ++r)
        {
            isPause();
            emptySet[r] = 1;
            blockSet[r] = 1;
        }
        for (int r = 0; r < row; ++r)
        {
            isPause();
            if (nonogramGraph[r][idx] == 1)
                emptySet[r] = 0;
            else if (nonogramGraph[r][idx] == 0)
                blockSet[r] = 0;
        }
    }

    int fillingCnt = 0;
    if (rowOrCol == _ROW_CHOOSE)
    {
        fillingCnt = rowPossibleFilling[idx];
        rowPossibleFilling[idx] = 0;
    }
    else
    {
        fillingCnt = colPossibleFilling[idx];
        colPossibleFilling[idx] = 0;
    }
    isPause();
    if (!m_isCanceled)
        enumAllOption(idx, rowOrCol, emptyGroup, 0, groupNum, empty_num, blockSet, emptySet);

    bool res = true;
    if ((rowOrCol == _ROW_CHOOSE && rowPossibleFilling[idx] == 0) || (rowOrCol == _COL_CHOOSE && colPossibleFilling[idx] == 0))
    {
        feasible = false;
        res = false;
    }
    else
    {
        if ((rowOrCol == _ROW_CHOOSE && fillingCnt == rowPossibleFilling[idx]) || (rowOrCol == _COL_CHOOSE && fillingCnt == colPossibleFilling[idx]))
            res = false;
        else
        {
            // filling the new definite square to block(1) or empty(0)
            if (rowOrCol == _ROW_CHOOSE)
            {
                for (int c = 0; c < col; ++c)
                {
                    isPause();
                    if (blockSet[c] == 1)
                        nonogramGraph[idx][c] = 1;
                    if (emptySet[c] == 1)
                        nonogramGraph[idx][c] = 0;
                }
            }
            else
            {
                for (int r = 0; r < row; ++r)
                {
                    isPause();
                    if (blockSet[r] == 1)
                        nonogramGraph[r][idx] = 1;
                    if (emptySet[r] == 1)
                        nonogramGraph[r][idx] = 0;
                }
            }
        }
    }
    isPause();
    delete[] emptyGroup;
    delete[] blockSet;
    delete[] emptySet;
    return res;
}

// this function is used to enumeration all the possible of filling option given the rowList_blockGroup at row / col idx
void nonogram::enumAllOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num, int *blockSet, int *emptySet)
{
    int empty_upper;
    // cause at least (rowList_blockGroup[idx].size() - 1) empty needed to insert between rowList_blockGroup[idx].size() groups
    if (rowOrCol == _ROW_CHOOSE)
        empty_upper = empty_num - (rowList_blockGroup_size[idx] - emptyGroupIdx - 1);
    // cause at least (colList_blockGroup[idx].size() - 1) empty needed to insert between colList_blockGroup[idx].size() groups
    else
        empty_upper = empty_num - (colList_blockGroup_size[idx] - emptyGroupIdx - 1);
    if (emptyGroupIdx == groupNum)
    {
        isPause();
        if (m_isCanceled)
            return;
        // put the number of empth squares left in the back of emptyGroup
        emptyGroup[emptyGroupIdx] = empty_num;
        enumOptionCheck(idx, rowOrCol, emptyGroup, blockSet, emptySet);
        return;
    }
    if (emptyGroupIdx == 0)
    {
        for (int cnt = 0; cnt <= empty_upper; ++cnt)
        {
            isPause();
            if (m_isCanceled)
                return;
            emptyGroup[0] = cnt;
            enumAllOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt, blockSet, emptySet);
        }
    }
    else if (emptyGroupIdx < groupNum)
    {
        // since we need to place empty squares between block groups, so the minimum is start from cnt = 1
        for (int cnt = 1; cnt <= empty_upper; ++cnt)
        {
            isPause();
            if (m_isCanceled)
                return;
            emptyGroup[emptyGroupIdx] = cnt;
            enumAllOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt, blockSet, emptySet);
        }
    }
}

// this function is used to check whether the current option is feasible, the method is to check whether it violates the the current filling situation in nonogramGraph
//  if it is feasible, then we mark blockSet[i] = 0 for i that is empty interval in this option, and emptySet[i] = 0 for i that is block interval in this option
void nonogram::enumOptionCheck(int idx, int rowOrCol, int *emptyGroup, int *blockSet, int *emptySet)
{
    // compare with the current filling situation with the rowEmptyGroup to see if this filling option is possible
    int s_idx = 0, e_idx = 0;

    int group_num;
    if (rowOrCol == _ROW_CHOOSE)
        group_num = rowList_blockGroup_size[idx];
    else
        group_num = colList_blockGroup_size[idx];
    for (int i = 0; i < group_num; ++i)
    {
        isPause();
        if (m_isCanceled)
            return;
        e_idx = s_idx + emptyGroup[i] - 1;
        // s_idx ~ e_idx is empty in this option, but if some idx between s_idx and e_idx is sure to be block, then we just return.
        for (int j = s_idx; j <= e_idx; ++j)
        {
            isPause();
            if (m_isCanceled)
                return;
            if ((rowOrCol == _ROW_CHOOSE && nonogramGraph[idx][j] == 1) || (rowOrCol == _COL_CHOOSE && nonogramGraph[j][idx] == 1))
                return;
        }
        s_idx = e_idx + 1;

        e_idx = s_idx + (rowOrCol == _ROW_CHOOSE ? rowList_blockGroup[idx][i] : colList_blockGroup[idx][i]) - 1;
        // s_idx ~ e_idx is block in this option, but if some idx between s_idx and e_idx is sure to be empty, then we just return.
        for (int j = s_idx; j <= e_idx; ++j)
        {
            isPause();
            if (m_isCanceled)
                return;
            if ((rowOrCol == _ROW_CHOOSE && nonogramGraph[idx][j] == 0) || (rowOrCol == _COL_CHOOSE && nonogramGraph[j][idx] == 0))
                return;
        }
        s_idx = e_idx + 1;
    }
    if (rowOrCol == _ROW_CHOOSE)
    {
        for (int j = s_idx; j < col; ++j)
        {
            isPause();
            if (m_isCanceled)
                return;
            if (nonogramGraph[idx][j] == 1)
                return;
        }
    }
    else
    {
        for (int j = s_idx; j < row; ++j)
        {
            isPause();
            if (m_isCanceled)
                return;
            if (nonogramGraph[j][idx] == 1)
                return;
        }
    }

    // if we make here, which means that this option is feasible,
    //  then we mark the interval(which is empty in this option) of blockSet = 0
    //  and mark the interval(which is block in this option) of emptySet = 0
    s_idx = 0, e_idx = 0;
    for (int i = 0; i < group_num; ++i)
    {
        isPause();
        if (m_isCanceled)
            return;
        e_idx = s_idx + emptyGroup[i] - 1;
        // s_idx ~ e_idx is empty in this option
        for (int j = s_idx; j <= e_idx; ++j)
            blockSet[j] = 0;
        s_idx = e_idx + 1;

        isPause();
        if (m_isCanceled)
            return;
        e_idx = s_idx + (rowOrCol == _ROW_CHOOSE ? rowList_blockGroup[idx][i] : colList_blockGroup[idx][i]) - 1;
        // s_idx ~ e_idx is block in this option
        for (int j = s_idx; j <= e_idx; ++j)
            emptySet[j] = 0;
        isPause();
        if (m_isCanceled)
            return;
        s_idx = e_idx + 1;
    }
    int _upper = rowOrCol == _ROW_CHOOSE ? col : row;
    for (int j = s_idx; j < _upper; ++j)
        blockSet[j] = 0;
    isPause();
    if (m_isCanceled)
        return;
    if (rowOrCol == _ROW_CHOOSE)
        rowPossibleFilling[idx]++;
    else
        colPossibleFilling[idx]++;
    return;
}
