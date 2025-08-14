#include "nonogramboard.h"
#include "aspectratio.h"

NonogramBoard::NonogramBoard(QWidget *parent) : QWidget(parent)
{
    stateChanged(STATE_INITIALIZAION);
}

void NonogramBoard::stateChanged(int stateChange)
{
    state = stateChange;
    switch (state)
    {
    case STATE_INITIALIZAION:
        initialization();
        break;
    case STATE_GENERATE_FILLING_TABLE:
        generateFillingTable();
        break;
    case STATE_SET_TABLE:
        settingTable();
        break;
    case STATE_SHOW_FILLING_TABLE:
        showingTable();
        break;
    case STATE_SOLVEING:
        nonogramSolving();
        break;
    case STATE_STOP_SOLVEING:
        nonogramStop();
        break;
    case STATE_GET_SOLVING_SUCCESS_OUTCOME:
        getSolvingSuccessOutcome();
        break;
    case STATE_GET_SOLVING_FAIL_OUTCOME:
        getSolvingFailOutcome();
        break;
    case STATE_DEFAULT:
        defaultState();
        break;
    }
}

void NonogramBoard::initialization()
{
    rowNumBox = new QSpinBox(this);
    colNumBox = new QSpinBox(this);
    rowNumBox->setRange(1, _ROWNUM_MAX);
    colNumBox->setRange(1, _COLNUM_MAX);
    rowNumBox->setValue(_DEFAULT_ROW_NUM);
    colNumBox->setValue(_DEFAULT_COL_NUM);

    rowNumLabel = new QLabel(tr("rows: "));
    colNumLabel = new QLabel(tr("cols: "));

    generateButton = new QPushButton(tr("&Generate"));
    connect(generateButton, &QPushButton::clicked, this, [this]()
            { stateChanged(STATE_GENERATE_FILLING_TABLE); });

    tableParentWidget = new AspectRatioWidget(this);
    connect(tableParentWidget, SIGNAL(redawCanvasSignal()), this, SLOT(canvasRedraw()));
    fillingTable = new QTableWidget(tableParentWidget);
    fillingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fillingTable->horizontalHeader()->setVisible(false);
    fillingTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fillingTable->verticalHeader()->setVisible(false);
    tableParentWidget->setAspectWidget(fillingTable);
    fillingTable->setVisible(false);

    // The issue you are facing with QLabel containing a QPixmap not zooming out beyond a certain size is typically because the
    // QLabel or QPixmap size is limiting the zoom, and QLabel does not automatically resize to accommodate zooming out beyond its initial size.
    canvasScrollArea = new QScrollArea(tableParentWidget); // Use a QScrollArea with QLabel inside to fix QLabel with QPixmap zooming limitations (by perplexity)
    canvas = new QLabel(canvasScrollArea);
    canvas->setSizePolicy(QSizePolicy::Ignored,
                          QSizePolicy::Ignored);
    canvas->setScaledContents(true); // make the content(Qpixmap) to transform with the Qlabel
    // canvas->setVisible(false);

    canvasScrollArea->setWidget(canvas);
    canvasScrollArea->setWidgetResizable(true);
    canvasScrollArea->setVisible(false);
    isCanvasSaved = true;

    addRowButton = new QPushButton(tr("&Add row"));
    connect(addRowButton, SIGNAL(clicked()), this, SLOT(tableAddRow()));
    addRowButton->setVisible(false);

    delRowButton = new QPushButton(tr("&Delete row"));
    connect(delRowButton, SIGNAL(clicked()), this, SLOT(tableDelRow()));
    delRowButton->setVisible(false);

    addColButton = new QPushButton(tr("&Add col"));
    connect(addColButton, SIGNAL(clicked()), this, SLOT(tableAddCol()));
    addColButton->setVisible(false);

    delColButton = new QPushButton(tr("&Delete col"));
    connect(delColButton, SIGNAL(clicked()), this, SLOT(tableDelCol()));
    delColButton->setVisible(false);

    setButton = new QPushButton(tr("&Set"));
    connect(setButton, &QPushButton::clicked, this, [this]()
            { stateChanged(STATE_SET_TABLE); });
    setButton->setVisible(false);

    modifyButton = new QPushButton(tr("&Modify"));
    connect(modifyButton, &QPushButton::clicked, this, [this]()
            { stateChanged(STATE_SHOW_FILLING_TABLE); });
    modifyButton->setVisible(false);

    startButton = new QPushButton(tr("&Start Solving"));
    connect(startButton, &QPushButton::clicked, this, [this]()
            { stateChanged(STATE_SOLVEING); });
    startButton->setVisible(false);

    stopButton = new QPushButton(tr("&Stop Solving"));
    connect(stopButton, &QPushButton::clicked, this, [this]()
            { stateChanged(STATE_STOP_SOLVEING); });
    stopButton->setVisible(false);

    topLayout = new QHBoxLayout;
    topLayout->addStretch();
    topLayout->addWidget(rowNumLabel);
    topLayout->addWidget(rowNumBox);
    topLayout->addWidget(colNumLabel);
    topLayout->addWidget(colNumBox);
    topLayout->addWidget(generateButton);
    topLayout->addStretch();

    rightLayout = new QVBoxLayout;
    rightLayout->addWidget(addRowButton);
    rightLayout->addWidget(delRowButton);
    rightLayout->addWidget(addColButton);
    rightLayout->addWidget(delColButton);
    rightLayout->addWidget(setButton);
    rightLayout->addWidget(modifyButton);
    rightLayout->addStretch(1);
    rightLayout->addWidget(startButton);
    rightLayout->addWidget(stopButton);

    gridLayout = new QGridLayout;
    gridLayout->addLayout(topLayout, 0, 0);
    // gridLayout->addWidget(fillingTable, 1, 0);
    gridLayout->addWidget(tableParentWidget, 1, 0);
    gridLayout->addLayout(rightLayout, 1, 1);
    setLayout(gridLayout);
}

void NonogramBoard::generateFillingTable()
{
    double cellSize;
    if (fillingTable->rowCount())
    {
        cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());
    }
    else
    {
        cellSize = CELL_SIZE_MIN;
    }
    rowNum = rowNumBox->value();
    colNum = colNumBox->value();

    fillingTableGenerateHelper(_DEFAULT_ROW_FILL_PART_NUM, _DEFAULT_COL_FILL_PART_NUM, nullptr, nullptr);

    tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount()) / static_cast<double>(fillingTable->columnCount()));
    tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));
    // QString tmp1 = QString::number(static_cast<double>(fillingTable->height()) / static_cast<double>(fillingTable->rowCount()));
    // QString tmp2 = QString::number(static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount()));
    // rowNumLabel->setText(tmp1);
    // colNumLabel->setText(tmp2);
    addRowButton->setVisible(true);
    delRowButton->setVisible(true);
    addColButton->setVisible(true);
    delColButton->setVisible(true);
    setButton->setVisible(true);
    modifyButton->setVisible(true);
    startButton->setVisible(true);
    stopButton->setVisible(true);

    addRowButton->setEnabled(true);
    delRowButton->setEnabled(true);
    addColButton->setEnabled(true);
    delColButton->setEnabled(true);
    setButton->setEnabled(true);
    modifyButton->setEnabled(false);
    startButton->setEnabled(false);
    stopButton->setEnabled(false);

    isCanvasSaved = true;
}

void NonogramBoard::settingTable()
{
    rowNumBox->setEnabled(false);
    colNumBox->setEnabled(false);
    generateButton->setEnabled(false);
    addRowButton->setEnabled(false);
    delRowButton->setEnabled(false);
    addColButton->setEnabled(false);
    delColButton->setEnabled(false);
    setButton->setEnabled(false);
    modifyButton->setEnabled(true);
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    fillingTable->setVisible(false);

    canvas->resize(fillingTable->width(), fillingTable->height());
    canvasDrawTable();
    // canvas->setVisible(true);
    canvasScrollArea->setVisible(true);
    isCanvasSaved = true;
    tableParentWidget->setAspectWidget(canvasScrollArea);
}

void NonogramBoard::showingTable()
{
    rowNumBox->setEnabled(true);
    colNumBox->setEnabled(true);
    generateButton->setEnabled(true);
    addRowButton->setEnabled(true);
    delRowButton->setEnabled(true);
    addColButton->setEnabled(true);
    delColButton->setEnabled(true);
    modifyButton->setEnabled(false);
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
    // canvas->setVisible(false); // this line has to be in front of fillingTable->setVisible(true), otherwise, if I press modify button, the window will enlarge immediately
    canvasScrollArea->setVisible(false);
    isCanvasSaved = true;

    // cause the outcome is store in the fillingTable through drawSpecificCell function. Use fillingReadOnly function to clear the outcome store in the fillingTable.
    int totalRowNum = fillingTable->rowCount();
    int totalColNum = fillingTable->columnCount();
    int start_col = totalColNum - colNum;
    int start_row = totalRowNum - rowNum;
    fillingReadOnly(start_row, totalRowNum - 1, start_col, totalColNum - 1);

    fillingTable->setVisible(true);
    tableParentWidget->setAspectWidget(fillingTable);
    if (gram != nullptr)
    {
        emit stopSolvingSignal();
    }
    else
    {
        setButton->setEnabled(true);
    }
}

void NonogramBoard::nonogramSolving()
{
    startButton->setEnabled(false);
    if (gram != nullptr)
    {
        emit restartSolvingSignal();
        stopButton->setEnabled(true);
        return;
    }
    vector<vector<int>> rowList(rowNum, vector<int>());
    vector<vector<int>> colList(colNum, vector<int>());
    int reason = generateBlockEmptyList(rowList, colList);
    if (reason)
    {
        modifyButton->setEnabled(true);
        if (reason == GENERATE_LIST_FAIL_NOT_INT)
            QMessageBox::about(this, tr("Error"),
                               tr("<p> Please enter positive integer.</p>"));
        else if (reason == GENERATE_LIST_FAIL_EXCEED)
            QMessageBox::about(this, tr("Error"),
                               tr("<p> The sum of the numbers at a specific row or column exceed the the number of columns or rows.</p>"));
    }
    else
    {
        gram = new nonogram;
        connect(gram, SIGNAL(nonogramIdxDefinite(int, int, int)), this, SLOT(drawSpecificCell(int, int, int)), Qt::BlockingQueuedConnection);
        connect(this, SIGNAL(pauseSolvingSignal()), gram, SLOT(pauseSolvingHandler()));
        connect(this, SIGNAL(restartSolvingSignal()), gram, SLOT(restartSolvingHandler()));
        connect(this, SIGNAL(stopSolvingSignal()), gram, SLOT(stopSolvingHandler()));
        connect(gram, SIGNAL(QmessageSend(QString, QString)), this, SLOT(QmessageReceive(QString, QString)));
        connect(gram, SIGNAL(clearFinished(int)), this, SLOT(clearFinishedHandler(int)));
        stopButton->setEnabled(true);
        gram->solvingPuzzle(rowList, colList);

        // here, get the width and height from tableParentWidget before solving, because if drawSpecificCell() use the width and height from fillingTable,
        // the image will be drew in the wrong position. (cause the width and height of fillingTable is the initial size)
        width_during_solving = tableParentWidget->width();
        height_during_solving = tableParentWidget->height();
        double aspect = height_during_solving / width_during_solving;
        double m_ratio = tableParentWidget->getRatio();
        if (aspect < m_ratio) // parent is too wide
        {
            width_during_solving = height_during_solving / m_ratio;
        }
        else // parent is too high
        {
            height_during_solving = width_during_solving * m_ratio;
        }
    }
}

void NonogramBoard::nonogramStop()
{
    stopButton->setEnabled(false);
    startButton->setEnabled(true);
    modifyButton->setEnabled(true);
    emit pauseSolvingSignal();
}

void NonogramBoard::getSolvingSuccessOutcome()
{
}

void NonogramBoard::getSolvingFailOutcome()
{
}

void NonogramBoard::defaultState()
{
    addRowButton->setVisible(false);
    delRowButton->setVisible(false);
    addColButton->setVisible(false);
    delColButton->setVisible(false);
    setButton->setVisible(false);
    modifyButton->setVisible(false);
    startButton->setVisible(false);
    stopButton->setVisible(false);
    // canvas->setVisible(false); // this line has to be in front of fillingTable->setVisible(true), otherwise, if I press modify button, the window will enlarge immediately
    canvasScrollArea->setVisible(false);
    fillingTable->setVisible(false);
    tableParentWidget->setAspectWidget(fillingTable);
    rowNumBox->setEnabled(true);
    colNumBox->setEnabled(true);
    generateButton->setEnabled(true);

    isCanvasSaved = true;

    emit defaultSignal();
}

void NonogramBoard::detectDataInputHandler(vector<vector<int>> _rowList, vector<vector<int>> _colList)
{
    setToDefault();

    vector<vector<int>> rowList = _rowList;
    vector<vector<int>> colList = _colList;
    rowNum = rowList.size() > 0 ? rowList.size() : 1;
    colNum = colList.size() > 0 ? colList.size() : 1;
    rowNumBox->setValue(rowNum);
    colNumBox->setValue(colNum);

    int rowFillNumber = 0;
    int colFillNumber = 0;
    for (int i = 0; i < rowList.size(); ++i)
    {
        rowFillNumber = max(rowFillNumber, static_cast<int>(rowList[i].size()));
    }
    for (int i = 0; i < colList.size(); ++i)
    {
        colFillNumber = max(colFillNumber, static_cast<int>(colList[i].size()));
    }

    fillingTableGenerateHelper(rowFillNumber, colFillNumber, &rowList, &colList);

    double cellSize;
    cellSize = static_cast<double>(fillingTable->width()) / static_cast<double>(fillingTable->columnCount());

    tableParentWidget->setRatio(static_cast<double>(fillingTable->rowCount()) / static_cast<double>(fillingTable->columnCount()));
    tableParentWidget->resize(static_cast<double>(fillingTable->columnCount() * cellSize), static_cast<double>(fillingTable->rowCount() * cellSize));

    addRowButton->setVisible(true);
    delRowButton->setVisible(true);
    addColButton->setVisible(true);
    delColButton->setVisible(true);
    setButton->setVisible(true);
    modifyButton->setVisible(true);
    startButton->setVisible(true);
    stopButton->setVisible(true);

    addRowButton->setEnabled(true);
    delRowButton->setEnabled(true);
    addColButton->setEnabled(true);
    delColButton->setEnabled(true);
    setButton->setEnabled(true);
    modifyButton->setEnabled(false);
    startButton->setEnabled(false);
    stopButton->setEnabled(false);
}

bool NonogramBoard::isInWorkingState()
{
    if (gram != nullptr)
        return true;
    return false;
}

bool NonogramBoard::hasOutcome()
{
    if (state == STATE_GET_SOLVING_SUCCESS_OUTCOME)
    {
        if (isCanvasSaved == false) // means the canvas hasn't been saved yet
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool NonogramBoard::saveOutcome(const QString &fileName, const char *fileFormat)
{
    if (canvas != nullptr)
    {
        QPixmap pix = canvas->pixmap();
        if (pix.save(fileName, fileFormat))
        {
            isCanvasSaved = true;
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

void NonogramBoard::setToDefault()
{
    if (isInWorkingState())
    {
        emit stopSolvingSignal();
    }
    else
    {
        stateChanged(STATE_DEFAULT);
    }
}