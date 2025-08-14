#define ML_USING_CPLUSPLUS_EN true

#if !ML_USING_CPLUSPLUS_EN
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#pragma pop_macro("slots")
#endif /*!ML_USING_CPLUSPLUS_EN*/
#include <QWidget>
#include <QMessageBox>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#ifndef _IMAGEDETECTOR
#define _IMAGEDETECTOR
#include <vector>
#include <iostream>

using namespace std;

class imagedetector : public QThread
{
    Q_OBJECT

    QString inputFileName;

private:
    void inputImageStartDetect();

public:
    imagedetector(QObject *parent = 0);
    ~imagedetector();
    // this function is for image detecting
    void detectingInputImage(QString &fileName);

#if !ML_USING_CPLUSPLUS_EN
    void TwoDlistTupleToTwoDVector_Int(PyObject *, vector<vector<int>> &);
    vector<int> listTupleToVector_Int(PyObject *);
#endif /*!ML_USING_CPLUSPLUS_EN*/

protected:
    void run();

signals:
    void QmessageSend(QString, QString);
    void detectOutputSend(vector<vector<int>> rowList, vector<vector<int>> colList);
};

#endif