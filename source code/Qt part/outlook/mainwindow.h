#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressDialog>

#include "../nonogramboard.h"
#include "../../image process part/C++ part/imagedetector.h"

class NonogramBoard;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    NonogramBoard *nonogramField;
    // QMenuBar *menuBar;

    QMenu *saveAsMenu;
    QMenu *fileMenu;
    QMenu *optionMenu;
    QMenu *helpMenu;

    QAction *openAct;
    QList<QAction *> saveAsActs;
    QAction *exitAct;
    QAction *backToDefaultAct;
    QAction *aboutAct;
    QAction *aboutQtAct;

    imagedetector *detector = nullptr;
    QProgressDialog *progressDialog;

    bool isWidgetDefault;

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void open();
    void save();
    void about();

    void widgetDefaultHandler();
    void QmessageReceive(QString, QString);
    void detectOutputHandler(vector<vector<int>> rowList, vector<vector<int>> colList);

signals:
    void detectDataInputSignal(vector<vector<int>>, vector<vector<int>>);

private:
    void createActions();
    void createMenus();
    bool maybeSaveOutCome();
    bool saveFile(const QByteArray &fileFormat);
};

#endif