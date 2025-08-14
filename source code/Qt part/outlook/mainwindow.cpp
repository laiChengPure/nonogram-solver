#include <QtGui>

#include "mainwindow.h"
#include "..\nonogramboard.h"

// ref: https://qtdocs.narod.ru/4.1.0/doc/html/widgets-scribble.html

MainWindow::MainWindow()
{
    nonogramField = new NonogramBoard;
    setCentralWidget(nonogramField);
    connect(nonogramField, SIGNAL(defaultSignal()), this, SLOT(widgetDefaultHandler()));
    connect(this, SIGNAL(detectDataInputSignal(vector<vector<int>>, vector<vector<int>>)), nonogramField, SLOT(detectDataInputHandler(vector<vector<int>>, vector<vector<int>>)));

    isWidgetDefault = false;
    createActions();
    createMenus();

    setWindowTitle(tr("Nonogran Solver"));
    setWindowIcon(QIcon(":/icons/nonogram_icon.png"));
    resize(500, 0);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSaveOutCome())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

// In the open() slot we first give the user the opportunity to save any modifications to the currently outcome image, before a new image is loaded into the nonogram field area.
// Then we ask the user to choose a file and we load the file in the nonogram field area.
void MainWindow::open()
{
    if (maybeSaveOutCome())
    {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open File"), QDir::currentPath());
        if (fileName.isEmpty()) // if the filename is empty, return
            return;
        QImage loadedImage;
        if (!loadedImage.load(fileName)) // if the file is not image, return
            return;

        openAct->setEnabled(false);

        isWidgetDefault = false;
        nonogramField->setToDefault();
        while (isWidgetDefault == false)
        {
        }

        progressDialog = new QProgressDialog("Detecting, please wait...", "Cancel", 0, 100, this);
        progressDialog->setWindowTitle("Working");
        progressDialog->setCancelButton(nullptr);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setRange(0, 0);        // Indeterminate mode
        progressDialog->setMinimumDuration(0); // Show it immediately
        progressDialog->show();

        resize(500, 0);
        // start detect image
        detector = new imagedetector;
        connect(detector, SIGNAL(QmessageSend(QString, QString)), this, SLOT(QmessageReceive(QString, QString)));
        connect(detector, SIGNAL(detectOutputSend(vector<vector<int>>, vector<vector<int>>)), this, SLOT(detectOutputHandler(vector<vector<int>>, vector<vector<int>>)));
        detector->detectingInputImage(fileName);
    }
}

void MainWindow::save()
{
    QAction *action = qobject_cast<QAction *>(sender()); // The first thing we need to do is to find out which action sent the signal using QObject::sender().
    QByteArray fileFormat = action->data().toByteArray();
    saveFile(fileFormat);
}

// about() slot to create a message box describing what the example is designed to show.
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Nonogram Solver"),
                       tr("<p>The <b>Nonogram Solver</b> is an application that can solve nonogran puzzle automatically."
                          "There are 2 method to input the nonogram data."
                          "First method is by entering the size of nonogram you want, then click the Generate button. Then, key the number into the table."
                          "Second method is click File -> Open, and select the nonogram image. After that, application will automatically generate the table for you."
                          "After getting the input table, click Set button and Start Solving button. It will start solving the puzzle."
                          "After a while, the outcome will show on the screen."
                          "If you want to save the outcome, you may click File -> Save.</p>"));
}

void MainWindow::widgetDefaultHandler()
{
    isWidgetDefault = true;
}

// recommand to read this article to know the function of action and menu
// https://medium.com/bucketing/pyside2-tutorial-ch15-qmenubar-qmenu-qaction-qtooltips-qstatustip-bcb5f765a6c5
void MainWindow::createActions()
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    foreach (QByteArray format, QImageReader::supportedImageFormats())
    {
        QString text = tr("%1...").arg(QString(format).toUpper());

        QAction *action = new QAction(text, this);
        action->setData(format);
        connect(action, SIGNAL(triggered()), this, SLOT(save()));
        saveAsActs.append(action);
    }

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    backToDefaultAct = new QAction(tr("&Back to Default"), this);
    backToDefaultAct->setShortcut(tr("Ctrl+L"));
    connect(backToDefaultAct, SIGNAL(triggered()), nonogramField, SLOT(setToDefault()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::createMenus()
{
    saveAsMenu = new QMenu(tr("&Save As"), this);
    foreach (QAction *action, saveAsActs)
        saveAsMenu->addAction(action);

    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addMenu(saveAsMenu);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    optionMenu = new QMenu(tr("&Options"), this);
    optionMenu->addAction(backToDefaultAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);

    // menuBar() is the member in QmainWindow?
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(optionMenu);
    menuBar()->addMenu(helpMenu);

    menuBar()->setStyleSheet("QMenuBar{background-color:GhostWhite}");
}

bool MainWindow::maybeSaveOutCome()
{
    if (nonogramField->hasOutcome())
    {
        int ret = QMessageBox::warning(this, tr("Nonogram"),
                                       tr("There is an outcome for nonogram.\n"
                                          "Do you want to save the outcome?"),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No,
                                       QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
        {
            return saveFile("png");
        }
        else if (ret == QMessageBox::No)
        {
            return true;
        }
        else // ret == QMessageBox::Cancel
        {
            return false;
        }
    }
    return true;
}

bool MainWindow::saveFile(const QByteArray &fileFormat)
{
    QString initialPath = QDir::currentPath() + "/untitled." + fileFormat;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    initialPath,
                                                    tr("%1 Files (*.%2);;All Files (*)")
                                                        .arg(QString(fileFormat.toUpper()))
                                                        .arg(QString(fileFormat)));
    if (fileName.isEmpty())
    {
        return false;
    }
    else
    {
        return nonogramField->saveOutcome(fileName, fileFormat);
    }
}

void MainWindow::QmessageReceive(QString title, QString text)
{
    QMessageBox::about(this, title, text);
    progressDialog->close();
    openAct->setEnabled(true);
}

void MainWindow::detectOutputHandler(vector<vector<int>> rowList, vector<vector<int>> colList)
{
    progressDialog->close();
    openAct->setEnabled(true);
    delete detector;
    detector = nullptr;
    emit detectDataInputSignal(rowList, colList);
}