#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QSpinBox>
#include <QColor>
#include <QHeaderView>
#include <QResizeEvent>
#include <QString>
#include <QMessageBox>
#include <vector>
#include "aspectratio.h"
#include "../solving part/src/nonogram.h"
#include <QThread>

#ifndef _NONOGRAMBOARD
#define _NONOGRAMBOARD

#define _ROWNUM_MAX 50
#define _COLNUM_MAX 50
#define _DEFAULT_ROW_NUM 10
#define _DEFAULT_COL_NUM 10

#define _DEFAULT_ROW_FILL_PART_NUM 2
#define _DEFAULT_COL_FILL_PART_NUM 2

#define STATE_INITIALIZAION 0
#define STATE_GENERATE_FILLING_TABLE 1
#define STATE_SET_TABLE 2
#define STATE_SHOW_FILLING_TABLE 3
#define STATE_SOLVEING 4
#define STATE_STOP_SOLVEING 5
#define STATE_GET_SOLVING_SUCCESS_OUTCOME 6
#define STATE_GET_SOLVING_FAIL_OUTCOME 7
#define STATE_DEFAULT 8

#define CELL_SIZE_MIN 10
#define CELL_SIZE_MAX 70

#define READONLY_COLOR QColor(173, 216, 230)

#define EMPTY_VAL 0
#define FILLING_VAL 1

#define UNSURE_COLOR QColor(196, 192, 192)
#define FILLING_COLOR QColor(0, 0, 0)
#define EMPTY_COLOR QColor(255, 255, 255)

#define GENERATE_LIST_SUCCESS 0
#define GENERATE_LIST_FAIL_NOT_INT 1
#define GENERATE_LIST_FAIL_EXCEED 2

#define FAIL 0
#define SUCCESS 1

class NonogramBoard : public QWidget
{
    Q_OBJECT

    int state;
    int rowNum;
    int colNum;

    QHBoxLayout *topLayout;
    QVBoxLayout *rightLayout;
    QGridLayout *gridLayout;

    QSpinBox *rowNumBox;
    QSpinBox *colNumBox;
    QLabel *rowNumLabel;
    QLabel *colNumLabel;

    QPushButton *generateButton;
    QPushButton *addRowButton;
    QPushButton *delRowButton;
    QPushButton *addColButton;
    QPushButton *delColButton;
    QPushButton *setButton;
    QPushButton *modifyButton;
    QPushButton *startButton;
    QPushButton *stopButton;

    QTableWidget *fillingTable;
    QScrollArea *canvasScrollArea;
    QLabel *canvas;

    double width_during_solving = 0;
    double height_during_solving = 0;
    bool isCanvasSaved;
    AspectRatioWidget *tableParentWidget;

    nonogram *gram = nullptr;

public:
    NonogramBoard(QWidget *parent = 0);

    bool isInWorkingState();
    bool hasOutcome();
    bool saveOutcome(const QString &fileName, const char *fileFormat);

public slots:
    void setToDefault();

private slots:
    void drawSpecificCell(int r, int c, int val);
    void canvasRedraw();
    void QmessageReceive(QString, QString);
    void clearFinishedHandler(int outcome);

    void stateChanged(int stateChange);
    void initialization();
    void generateFillingTable();
    void settingTable();
    void showingTable();
    void nonogramSolving();
    void nonogramStop();
    void getSolvingSuccessOutcome();
    void getSolvingFailOutcome();
    void defaultState();
    void detectDataInputHandler(vector<vector<int>>, vector<vector<int>>);

    void tableAddRow();
    void tableDelRow();
    void tableAddCol();
    void tableDelCol();
    void fillingTableGenerateHelper(int rowFillNumber, int colFillNumber, vector<vector<int>> *rowList, vector<vector<int>> *colList);
    void fillingTableFillInputDataHelper(vector<vector<int>> &rowList, vector<vector<int>> &colList, int tableUpperRowIdx, int tableUpperColIdx);
    void fillingReadOnly(int row1, int row2, int col1, int col2);
    void setTableMinMaxSize();
    void canvasDrawTable();
    int generateBlockEmptyList(vector<vector<int>> &rowList, vector<vector<int>> &colList);

signals:
    void pauseSolvingSignal();
    void restartSolvingSignal();
    void stopSolvingSignal();
    void defaultSignal();
};

#endif