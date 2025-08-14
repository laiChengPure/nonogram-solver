#include "nonogram.h"

bool nonogram::isFinished(int idx, int rowOrCol)
{
    if (rowOrCol == _ROW_CHOOSE)
    {
        if (rowPossibleFilling[idx] > 1)
            return false;
    }
    else
    {
        if (colPossibleFilling[idx] > 1)
            return false;
    }
    return true;
}

bool nonogram::isAllRowColFinished()
{
    int i;
    for (i = 0; i < row >> 5; ++i)
    {
        isPause();
        if (isRowFinished[i] != 0xffffffff)
            return false;
    }
    if ((row & 0b11111) != 0)
        if ((isRowFinished[i] & isRowFinishedMask) != (0xffffffff & isRowFinishedMask))
            return false;
    for (i = 0; i < col >> 5; ++i)
    {
        isPause();
        if (isColFinished[i] != 0xffffffff)
            return false;
    }
    if ((col & 0b11111) != 0)
        if ((isColFinished[i] & isColFinishedMask) != (0xffffffff & isColFinishedMask))
            return false;

    return true;
}

void nonogram::setRowOrColFinished(int idx, int rowOrCol)
{
    if (rowOrCol == _ROW_CHOOSE)
        isRowFinished[idx >> 5] |= 1 << (idx & 0b11111);
    else
        isColFinished[idx >> 5] |= 1 << (idx & 0b11111);
}

void nonogram::clearRowOrColFinished(int idx, int rowOrCol)
{
    if (rowOrCol == _ROW_CHOOSE)
        isRowFinished[idx >> 5] &= ~(1 << (idx & 0b11111));
    else
        isColFinished[idx >> 5] &= ~(1 << (idx & 0b11111));
}

bool nonogram::isRowOrColFinished(int idx, int rowOrCol)
{
    if (rowOrCol == _ROW_CHOOSE)
    {
        if (isRowFinished[idx >> 5] & (1 << (idx & 0b11111)))
            return true;
        else
        {
            if (rowPossibleFilling[idx] != 1)
                return false;
            return true;
        }
    }
    else
    {
        if (isColFinished[idx >> 5] & (1 << (idx & 0b11111)))
            return true;
        else
        {
            if (colPossibleFilling[idx] != 1)
                return false;
            return true;
        }
    }
}