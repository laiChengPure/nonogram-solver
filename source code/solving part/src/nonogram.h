#include <QWidget>
#include <QMessageBox>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#ifndef _NONOGRAM
#define _NONOGRAM

#define _ROW_CHOOSE 0
#define _COL_CHOOSE 1

#define FAIL 0
#define SUCCESS 1

#include <vector>
#include <iostream>
using namespace std;

// if a square is block: it will be filled with 1, if a square is empty: it will be filled with 0
class nonogram : public QThread
{
    Q_OBJECT

    bool feasible = true;

    int row = 0;
    int col = 0;
    int **rowList_blockGroup;     // used to store the block group in each row
    int *rowList_blockGroup_size; // used to store the number of block group in each row
    int *rowList_totalBlock;      // total number of block squares in each row
    int **colList_blockGroup;     // used to store the block group in each column
    int *colList_blockGroup_size; // used to store the number of block group in each col
    int *colList_totalBlock;      // total number of block squares in each column

    int *rowPossibleFilling; // this is used to store the total possible filling method of each row
    int *colPossibleFilling; // this is used to store the total possible filling method of each col

    int *isRowFinished; // whether a certain row has finished filling
    int *isColFinished; // whether a certain col has finished filling
    int isRowFinishedMask;
    int isColFinishedMask;

    bool allFinished;

    int **nonogramGraph; // the final nonogram will be present here

    // these variable are used for Qt part to pause or stop execute
    std::atomic_bool m_isPaused;
    std::atomic_bool m_isCanceled;
    QMutex mutex;
    QWaitCondition condition;

public:
    nonogram(QObject *parent = 0);
    ~nonogram();
    // this function is for solving nonogram
    void solvingPuzzle(vector<vector<int>> &rowList, vector<vector<int>> &colList);
    void nonogramSolve();
    void nonogramSetInput();

public slots:
    // for receiving "restart" and "pause solving" and "stop solving" signal
    void pauseSolvingHandler();
    void restartSolvingHandler();
    void stopSolvingHandler();

protected:
    void run();

signals:
    void nonogramIdxDefinite(int, int, int);
    void QmessageSend(QString, QString);
    void clearFinished(int);

private:
    void errorHandler();
    void finishedHandler();
    void printNonogram();
    void printNonogram(int idx, int rowOrCol);

    // the source of input
    void nonogramSetInput(vector<vector<int>> &rowList, vector<vector<int>> &colList);
    void typingInput();
    void fileInput();
    void groupListInput(vector<vector<int>> &rowList, vector<vector<int>> &colList);

    // step 1
    //  these functions is for enumerate all the possible filling option based on the block group
    bool intersectOption(int idx, int rowOrCol);
    void enumAllOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num, int *blockSet, int *emptySet);
    void enumOptionCheck(int idx, int rowOrCol, int *emptyGroup, int *blockSet, int *emptySet);
    void enumerateStep();

    // step 2
    void guessingStep();
    void guessingOption_pre(int idx, int rowOrCol);
    void guessingOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num);
    bool guessingOptionCheck(int idx, int rowOrCol, int *emptyGroup);
    void guessingSet(int idx, int rowOrCol, int *emptyGroup);
    //---//
    void guessing_FindIntersect();
    bool guessing_IntersectOption(int idx, int rowOrCol);
    void guessing_AllOption(int idx, int rowOrCol, int *emptyGroup, int emptyGroupIdx, int const groupNum, int empty_num, int *blockSet, int *emptySet);
    void guessing_OptionCheck(int idx, int rowOrCol, int *emptyGroup, int *blockSet, int *emptySet);

    // is finished functions
    bool isFinished(int idx, int rowOrCol);
    bool isAllRowColFinished();
    void setRowOrColFinished(int idx, int rowOrCol);
    void clearRowOrColFinished(int idx, int rowOrCol);
    bool isRowOrColFinished(int idx, int rowOrCol);

    // delete helper func
    void free2Darray(int **temp, int size);
    void freeMemberHandler(int outcome);

    // for Qt part to determine whether this thread to stop
    void isPause();
};

#endif