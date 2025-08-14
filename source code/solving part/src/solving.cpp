#include "nonogram.h"

void nonogram::run()
{
    nonogramSolve();
}

void nonogram::nonogramSolve()
{
    //-----//
    // step 1, list all the option to find intersection
    //-----//
    enumerateStep();
    if (m_isCanceled)
    {
        freeMemberHandler(FAIL);
        return;
    }
    if (feasible == false)
    {
        errorHandler();
        freeMemberHandler(FAIL);
        return;
    }
    if (allFinished)
    {
        cout << "finish" << endl;
        finishedHandler();
        freeMemberHandler(SUCCESS);
        return;
    }
    cout << "guessing" << endl;
    //-----//
    // step 2, Guessing step
    //-----//
    guessingStep();
    if (m_isCanceled)
    {
        freeMemberHandler(FAIL);
        return;
    }
    if (allFinished)
    {
        finishedHandler();
        freeMemberHandler(SUCCESS);
        return;
    }
    errorHandler();
    freeMemberHandler(FAIL);
}

void nonogram::errorHandler()
{
    // QMessageBox::about(this, tr("Error"),
    //                    tr("<p> NO solution.</p>"));
    emit QmessageSend(tr("Error"), tr("<p> NO solution.</p>"));
}

void nonogram::finishedHandler()
{
    // QMessageBox::about(this, tr("Finished"),
    //                    tr("<p> FINISHED.</p>"));
    printNonogram();
    emit QmessageSend(tr("Finished"), tr("<p> FINISHED.</p>"));
}

void nonogram::printNonogram()
{
    // for (int i = 0; i < row; ++i)
    // {
    //     for (int j = 0; j < col; ++j)
    //     {
    //         emit nonogramIdxDefinite(i, j, nonogramGraph[i][j]);
    //     }
    // }
}

void nonogram::printNonogram(int idx, int rowOrCol)
{
    if (rowOrCol == _ROW_CHOOSE)
    {
        for (int j = 0; j < col; ++j)
        {
            isPause();
            if (nonogramGraph[idx][j] != -1)
                emit nonogramIdxDefinite(idx, j, nonogramGraph[idx][j]);
        }
    }
    else
    {
        for (int j = 0; j < row; ++j)
        {
            isPause();
            if (nonogramGraph[j][idx] != -1)
                emit nonogramIdxDefinite(j, idx, nonogramGraph[j][idx]);
        }
    }
}

//----------//
void nonogram::isPause()
{
    if (m_isPaused)
    {
        mutex.lock();
        condition.wait(&mutex);
        mutex.unlock();
    }
}

void nonogram::pauseSolvingHandler()
{
    m_isPaused = true;
}

void nonogram::restartSolvingHandler()
{
    m_isPaused = false;
    if (isRunning())
        condition.wakeAll();
}

void nonogram::stopSolvingHandler()
{
    m_isCanceled = true;
    restartSolvingHandler();
}

void nonogram::freeMemberHandler(int outcome)
{
    free2Darray(rowList_blockGroup, row);
    delete[] rowList_blockGroup_size;
    delete[] rowList_totalBlock;
    free2Darray(colList_blockGroup, col);
    delete[] colList_blockGroup_size;
    delete[] colList_totalBlock;

    delete[] rowPossibleFilling;
    delete[] colPossibleFilling;

    if (row > 32)
        delete[] isRowFinished;
    else
        delete isRowFinished;
    if (col > 32)
        delete[] isColFinished;
    else
        delete isColFinished;
    free2Darray(nonogramGraph, row);
    emit clearFinished(outcome);
}