#include "nonogram.h"

// this function is for guessing step
void nonogram::guessingStep()
{
    // enumerate all the possible option and find the intersect,by doing this, we can fill definite value in the nonogram
    // till there is no change for all rows and cols, it can use sameCnt >= row + column to determine

    for (int i = 0; i < row; ++i)
    {
        isPause();
        if (allFinished || m_isCanceled)
            return;
        if (isRowOrColFinished(i, _ROW_CHOOSE))
        {
            setRowOrColFinished(i, _ROW_CHOOSE);
            continue;
        }
        guessingOption_pre(i, _ROW_CHOOSE);
        if (feasible == false || m_isCanceled)
            return;
    }
    for (int j = 0; j < col; ++j)
    {
        isPause();
        if (allFinished || m_isCanceled)
            return;
        if (isRowOrColFinished(j, _COL_CHOOSE))
        {
            setRowOrColFinished(j, _COL_CHOOSE);
            continue;
        }
        guessingOption_pre(j, _COL_CHOOSE);
        if (feasible == false || m_isCanceled)
            return;
    }
}

void nonogram::guessingOption_pre(int idx, int rowOrCol)
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
    if (m_isCanceled)
        return;
    // temp vector to record group(for consecutive empty square / group) in this row / col idx
    // since groupNum = number of group of block squares, so the number of group of empty squares is cellNum + 1
    int *emptyGroup = new int[groupNum + 1];

    guessingOption(idx, rowOrCol, emptyGroup, 0, groupNum, empty_num);
    return;
}

// this function is used at guessing step, find possible filling option given the rowList_blockGroup at row / col idx, and attempt this as the correct filling and keep going
// if we find that it is not possible to fill the whole nonogram at last, then we pick another possible filling option and do the same thing...
void nonogram::guessingOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num)
{
    int empty_upper;
    // cause at least (rowList_blockGroup[idx].size() - 1) empty needed to insert between rowList_blockGroup[idx].size() groups
    if (rowOrCol == _ROW_CHOOSE)
        empty_upper = empty_num - (rowList_blockGroup_size[idx] - emptyGroupIdx - 1);
    // cause at least (colList_blockGroup[idx].size() - 1) empty needed to insert between colList_blockGroup[idx].size() groups
    else
        empty_upper = empty_num - (colList_blockGroup_size[idx] - emptyGroupIdx - 1);

    isPause();
    if (m_isCanceled)
        return;
    if (emptyGroupIdx == 0)
    {
        for (int cnt = 0; cnt <= empty_upper; ++cnt)
        {
            isPause();
            if (m_isCanceled)
                return;
            feasible = true;
            emptyGroup[0] = cnt;
            guessingOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt);
            if (allFinished)
                return;
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
            feasible = true;
            emptyGroup[emptyGroupIdx] = cnt;
            guessingOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt);
            if (allFinished)
                return;
        }
    }
    else
    {
        isPause();
        if (m_isCanceled)
            return;
        // put the number of empth squares left in the back of emptyGroup
        emptyGroup[emptyGroupIdx] = empty_num;
        if (guessingOptionCheck(idx, rowOrCol, emptyGroup))
        {
            // copy the original nonogram to temp in case that the assumption is wrong and we can restore the original nonogram by temp
            int **temp = new int *[row];
            for (int i = 0; i < row; ++i)
            {
                isPause();
                temp[i] = new int[col];
                for (int j = 0; j < col; ++j)
                {
                    isPause();
                    if (m_isCanceled)
                    {
                        free2Darray(temp, row);
                        return;
                    }
                    temp[i][j] = nonogramGraph[i][j];
                }
            }
            if (m_isCanceled)
            {
                free2Darray(temp, row);
                return;
            }
            guessingSet(idx, rowOrCol, emptyGroup);
            printNonogram(idx, rowOrCol);
            setRowOrColFinished(idx, rowOrCol);
            if (m_isCanceled || isAllRowColFinished())
            {
                allFinished = true;
                free2Darray(temp, row);
                return;
            }
            isPause();
            if (m_isCanceled)
            {
                free2Darray(temp, row);
                return;
            }
            // go to intersect step
            guessing_FindIntersect();

            if (m_isCanceled || allFinished)
            {
                free2Darray(temp, row);
                return;
            }

            isPause();
            if (m_isCanceled)
            {
                free2Darray(temp, row);
                return;
            }
            // retrieve
            clearRowOrColFinished(idx, rowOrCol);
            for (int i = 0; i < row; ++i)
            {
                isPause();
                if (m_isCanceled)
                {
                    free2Darray(temp, row);
                    return;
                }
                for (int j = 0; j < col; ++j)
                {
                    isPause();
                    nonogramGraph[i][j] = temp[i][j];
                }
            }
            free2Darray(temp, row);
        }
    }
}

// this function is used to check whether the current option is feasible, the method is to check whether it violates the the current filling situation in nonogramGraph
bool nonogram::guessingOptionCheck(int idx, int rowOrCol, int *emptyGroup)
{
    // compare with the current filling situation with the rowEmptyGroup to see if this filling option is possible
    int s_idx = 0, e_idx = 0;
    isPause();
    int group_num;
    if (rowOrCol == _ROW_CHOOSE)
        group_num = rowList_blockGroup_size[idx];
    else
        group_num = colList_blockGroup_size[idx];
    for (int i = 0; i < group_num; ++i)
    {
        isPause();
        if (m_isCanceled)
            return false;
        e_idx = s_idx + emptyGroup[i] - 1;
        // s_idx ~ e_idx is empty in this option, but if some idx between s_idx and e_idx is sure to be block, then we just return.
        for (int j = s_idx; j <= e_idx; ++j)
        {
            isPause();
            if (m_isCanceled)
                return false;
            if ((rowOrCol == _ROW_CHOOSE && nonogramGraph[idx][j] == 1) || (rowOrCol == _COL_CHOOSE && nonogramGraph[j][idx] == 1))
                return false;
        }
        s_idx = e_idx + 1;

        e_idx = s_idx + (rowOrCol == _ROW_CHOOSE ? rowList_blockGroup[idx][i] : colList_blockGroup[idx][i]) - 1;
        // s_idx ~ e_idx is block in this option, but if some idx between s_idx and e_idx is sure to be empty, then we just return.
        for (int j = s_idx; j <= e_idx; ++j)
        {
            isPause();
            if (m_isCanceled)
                return false;
            if ((rowOrCol == _ROW_CHOOSE && nonogramGraph[idx][j] == 0) || (rowOrCol == _COL_CHOOSE && nonogramGraph[j][idx] == 0))
                return false;
        }
        s_idx = e_idx + 1;
    }
    if (rowOrCol == _ROW_CHOOSE)
    {
        for (int j = s_idx; j < col; ++j)
        {
            isPause();
            if (m_isCanceled)
                return false;
            if (nonogramGraph[idx][j] == 1)
                return false;
        }
    }
    else
    {
        for (int j = s_idx; j < row; ++j)
        {
            isPause();
            if (m_isCanceled)
                return false;
            if (nonogramGraph[j][idx] == 1)
                return false;
        }
    }
    return true;
}

// if it is feasible, then we assume this is the right filling, then we mark 0 and 1 based on the empty group and block group
void nonogram::guessingSet(int idx, int rowOrCol, int *emptyGroup)
{
    // if we make here, which means that this option is feasible
    // assume this option is the correct one
    int s_idx = 0, e_idx = 0;
    int group_num;
    if (rowOrCol == _ROW_CHOOSE)
        group_num = rowList_blockGroup_size[idx];
    else
        group_num = colList_blockGroup_size[idx];
    isPause();
    if (m_isCanceled)
        return;
    if (rowOrCol == _ROW_CHOOSE)
    {
        for (int i = 0; i < group_num; ++i)
        {
            isPause();
            if (m_isCanceled)
                return;
            e_idx = s_idx + emptyGroup[i] - 1;
            // s_idx ~ e_idx is empty in this option
            for (int j = s_idx; j <= e_idx; ++j)
                nonogramGraph[idx][j] = 0;
            s_idx = e_idx + 1;

            e_idx = s_idx + rowList_blockGroup[idx][i] - 1;
            // s_idx ~ e_idx is block in this option
            for (int j = s_idx; j <= e_idx; ++j)
                nonogramGraph[idx][j] = 1;
            s_idx = e_idx + 1;
        }
        isPause();
        if (m_isCanceled)
            return;
        for (int j = s_idx; j < col; ++j)
            nonogramGraph[idx][j] = 0;
    }
    else
    {
        for (int i = 0; i < group_num; ++i)
        {
            isPause();
            if (m_isCanceled)
                return;
            e_idx = s_idx + emptyGroup[i] - 1;
            // s_idx ~ e_idx is empty in this option
            for (int j = s_idx; j <= e_idx; ++j)
                nonogramGraph[j][idx] = 0;
            s_idx = e_idx + 1;

            e_idx = s_idx + colList_blockGroup[idx][i] - 1;
            // s_idx ~ e_idx is block in this option
            for (int j = s_idx; j <= e_idx; ++j)
                nonogramGraph[j][idx] = 1;
            s_idx = e_idx + 1;
        }
        isPause();
        if (m_isCanceled)
            return;
        for (int j = s_idx; j < row; ++j)
            nonogramGraph[j][idx] = 0;
    }

    return;
}

//----------//
// after set the guessing option, we find the intersect

// this function is used for finding the intersect at the guessing step, if nothing new change, then we have to guess again
void nonogram::guessing_FindIntersect()
{
    int sameCnt = 0; // if sameCnt >= row + col, which means that all the rowws and columns do not change, so we can go to the next step
    // enumerate all the possible option and find the intersect,by doing this, we can fill definite value in the nonogram
    // till there is no change for all rows and cols, it can use sameCnt >= row + column to determine

    // copy the isFinished row and col array first
    int *isRowFinished_temp = new int[(row >> 5) + (((row & 0b11111) == 0) ? 0 : 1)];
    int *isColFinished_temp = new int[(col >> 5) + (((col & 0b11111) == 0) ? 0 : 1)];
    for (int i = 0; i < (row >> 5) + (((row & 0b11111) == 0) ? 0 : 1); ++i)
        isRowFinished_temp[i] = isRowFinished[i];
    for (int i = 0; i < (col >> 5) + (((col & 0b11111) == 0) ? 0 : 1); ++i)
        isColFinished_temp[i] = isColFinished[i];
    while (1)
    {
        for (int i = 0; i < row; ++i)
        {
            isPause();
            if (feasible == false)
                break;
            if (isRowOrColFinished(i, _ROW_CHOOSE))
            {
                setRowOrColFinished(i, _ROW_CHOOSE);
                ++sameCnt;
                continue;
            }
            isPause();

            // if return true, means something change
            if (guessing_IntersectOption(i, _ROW_CHOOSE))
            {
                sameCnt = 0;
                printNonogram(i, _ROW_CHOOSE);
                if (isRowOrColFinished(i, _ROW_CHOOSE))
                    setRowOrColFinished(i, _ROW_CHOOSE);
            }
            else
                ++sameCnt;
            isPause();
            if (m_isCanceled)
            {
                if (row > 32)
                    delete[] isRowFinished_temp;
                else
                    delete isRowFinished_temp;
                if (col > 32)
                    delete[] isColFinished_temp;
                else
                    delete isColFinished_temp;
                return;
            }
            if (sameCnt >= row + col)
                break;
        }
        isPause();
        // printNonogram();
        if (feasible == false)
            break;
        if (sameCnt >= row + col)
            break;
        for (int j = 0; j < col; ++j)
        {
            isPause();
            if (m_isCanceled)
            {
                if (row > 32)
                    delete[] isRowFinished_temp;
                else
                    delete isRowFinished_temp;
                if (col > 32)
                    delete[] isColFinished_temp;
                else
                    delete isColFinished_temp;
                return;
            }
            if (feasible == false)
                break;
            if (isRowOrColFinished(j, _COL_CHOOSE))
            {
                setRowOrColFinished(j, _COL_CHOOSE);
                ++sameCnt;
                continue;
            }
            isPause();
            if (m_isCanceled)
            {
                if (row > 32)
                    delete[] isRowFinished_temp;
                else
                    delete isRowFinished_temp;
                if (col > 32)
                    delete[] isColFinished_temp;
                else
                    delete isColFinished_temp;
                return;
            }
            // if return true, means something change
            if (guessing_IntersectOption(j, _COL_CHOOSE))
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
        if (m_isCanceled)
        {
            if (row > 32)
                delete[] isRowFinished_temp;
            else
                delete isRowFinished_temp;
            if (col > 32)
                delete[] isColFinished_temp;
            else
                delete isColFinished_temp;
            return;
        }
        if (feasible == false)
            break;
        if (sameCnt >= row + col)
            break;
    }
    // every thing finished
    if (m_isCanceled || isAllRowColFinished())
    {
        allFinished = true;
        if (row > 32)
            delete[] isRowFinished_temp;
        else
            delete isRowFinished_temp;
        if (col > 32)
            delete[] isColFinished_temp;
        else
            delete isColFinished_temp;
        return;
    }
    isPause();
    // have to enter guessing part
    if (feasible)
        guessingStep();

    // if we come from the guessing part and the allFinished is true, then we can just return
    if (m_isCanceled || allFinished)
    {
        if (row > 32)
            delete[] isRowFinished_temp;
        else
            delete isRowFinished_temp;
        if (col > 32)
            delete[] isColFinished_temp;
        else
            delete isColFinished_temp;
        return;
    }
    isPause();
    // if we make here, means the upper layer guessing part is false, then we have to retrieve the isFinished row and col array
    for (int i = 0; i < (row >> 5) + (((row & 0b11111) == 0) ? 0 : 1); ++i)
        isRowFinished[i] = isRowFinished_temp[i];
    for (int i = 0; i < (col >> 5) + (((col & 0b11111) == 0) ? 0 : 1); ++i)
        isColFinished[i] = isColFinished_temp[i];
    isPause();
    if (row > 32)
        delete[] isRowFinished_temp;
    else
        delete isRowFinished_temp;
    if (col > 32)
        delete[] isColFinished_temp;
    else
        delete isColFinished_temp;
    return;
}

bool nonogram::guessing_IntersectOption(int idx, int rowOrCol)
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
    if (m_isCanceled)
        return false;
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
            if (m_isCanceled)
            {
                delete[] emptyGroup;
                delete[] blockSet;
                delete[] emptySet;
                return false;
            }
            emptySet[c] = 1;
            blockSet[c] = 1;
        }
        for (int c = 0; c < col; ++c)
        {
            isPause();
            if (m_isCanceled)
            {
                delete[] emptyGroup;
                delete[] blockSet;
                delete[] emptySet;
                return false;
            }
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
            if (m_isCanceled)
            {
                delete[] emptyGroup;
                delete[] blockSet;
                delete[] emptySet;
                return false;
            }
            emptySet[r] = 1;
            blockSet[r] = 1;
        }
        for (int r = 0; r < row; ++r)
        {
            isPause();
            if (m_isCanceled)
            {
                delete[] emptyGroup;
                delete[] blockSet;
                delete[] emptySet;
                return false;
            }
            if (nonogramGraph[r][idx] == 1)
                emptySet[r] = 0;
            else if (nonogramGraph[r][idx] == 0)
                blockSet[r] = 0;
        }
    }
    isPause();
    if (m_isCanceled)
    {
        delete[] emptyGroup;
        delete[] blockSet;
        delete[] emptySet;
        return false;
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
    if (m_isCanceled)
    {
        delete[] emptyGroup;
        delete[] blockSet;
        delete[] emptySet;
        return false;
    }
    guessing_AllOption(idx, rowOrCol, emptyGroup, 0, groupNum, empty_num, blockSet, emptySet);

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
                    if (m_isCanceled)
                    {
                        delete[] emptyGroup;
                        delete[] blockSet;
                        delete[] emptySet;
                        return false;
                    }
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
                    if (m_isCanceled)
                    {
                        delete[] emptyGroup;
                        delete[] blockSet;
                        delete[] emptySet;
                        return false;
                    }
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
void nonogram::guessing_AllOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num, int *blockSet, int *emptySet)
{
    int empty_upper;
    // cause at least (rowList_blockGroup[idx].size() - 1) empty needed to insert between rowList_blockGroup[idx].size() groups
    if (rowOrCol == _ROW_CHOOSE)
        empty_upper = empty_num - (rowList_blockGroup_size[idx] - emptyGroupIdx - 1);
    // cause at least (colList_blockGroup[idx].size() - 1) empty needed to insert between colList_blockGroup[idx].size() groups
    else
        empty_upper = empty_num - (colList_blockGroup_size[idx] - emptyGroupIdx - 1);
    isPause();
    if (m_isCanceled)
        return;
    if (emptyGroupIdx == 0)
    {
        for (int cnt = 0; cnt <= empty_upper; ++cnt)
        {
            isPause();
            if (m_isCanceled)
                return;
            emptyGroup[0] = cnt;
            guessing_AllOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt, blockSet, emptySet);
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
            guessing_AllOption(idx, rowOrCol, emptyGroup, emptyGroupIdx + 1, groupNum, empty_num - cnt, blockSet, emptySet);
        }
    }
    else
    {
        isPause();
        if (m_isCanceled)
            return;
        // put the number of empth squares left in the back of emptyGroup
        emptyGroup[emptyGroupIdx] = empty_num;
        guessing_OptionCheck(idx, rowOrCol, emptyGroup, blockSet, emptySet);
    }
}

// this function is used to check whether the current option is feasible, the method is to check whether it violates the the current filling situation in nonogramGraph
//  if it is feasible, then we mark blockSet[i] = 0 for i that is empty interval in this option, and emptySet[i] = 0 for i that is block interval in this option
void nonogram::guessing_OptionCheck(int idx, int rowOrCol, int *emptyGroup, int *blockSet, int *emptySet)
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
    isPause();
    if (m_isCanceled)
        return;
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

        e_idx = s_idx + (rowOrCol == _ROW_CHOOSE ? rowList_blockGroup[idx][i] : colList_blockGroup[idx][i]) - 1;
        // s_idx ~ e_idx is block in this option
        for (int j = s_idx; j <= e_idx; ++j)
            emptySet[j] = 0;
        s_idx = e_idx + 1;
    }
    isPause();
    if (m_isCanceled)
        return;
    int _upper = rowOrCol == _ROW_CHOOSE ? col : row;
    for (int j = s_idx; j < _upper; ++j)
        blockSet[j] = 0;
    if (rowOrCol == _ROW_CHOOSE)
        rowPossibleFilling[idx]++;
    else
        colPossibleFilling[idx]++;
    return;
}