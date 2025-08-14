#include "imagedetector.h"
#if ML_USING_CPLUSPLUS_EN
#include "../ML CPP version/CPP_ver.h"
#else /*ML_USING_CPLUSPLUS_EN*/
#include <Python.h>
#include "pythreadstatelock.h"
#endif /*ML_USING_CPLUSPLUS_EN*/
#include <iostream>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;
imagedetector::imagedetector(QObject *parent) : QThread(parent)
{
}

imagedetector::~imagedetector()
{
}

void imagedetector::detectingInputImage(QString &fileName)
{
    inputFileName = fileName;
    start(LowPriority);
}

void imagedetector::run()
{
    inputImageStartDetect();
}

void imagedetector::inputImageStartDetect()
{
#if ML_USING_CPLUSPLUS_EN
    vector<vector<int>> rowList;
    vector<vector<int>> colList;
    string str_inputFileName = inputFileName.toStdString();
    nonogramHintDetect(str_inputFileName, rowList, colList);
#else  /*ML_USING_CPLUSPLUS_EN*/

    Py_Initialize();
    if (!Py_IsInitialized())
    {
        emit QmessageSend(tr("Error"), tr("<p> Fail to init python.</p>"));
        return;
    }

    PyThreadStateLock PyThreadLock;

    PyRun_SimpleString("import sys");
    // append the path of ptthon script
    fs::path cur_path = fs::current_path();
    fs::path parent_path = cur_path.parent_path();
    std::string str_parent_path = parent_path.generic_string();
    std::string str_target_path = str_parent_path + "/image process part/ML python version";
    std::string path = "sys.path.append('" + str_target_path + "')";
    PyRun_SimpleString(&path[0]);

    PyObject *pModule = PyImport_ImportModule("main_code");
    if (!pModule)
    {
        emit QmessageSend(tr("Error"), tr("<p> Fail to load python module.</p>"));
        Py_Finalize();
        return;
    }
    PyObject *pFunc = PyObject_GetAttrString(pModule, "nonogramHintDetect");
    if (!pFunc)
    {
        emit QmessageSend(tr("Error"), tr("<p> Can't find func.</p>"));
        Py_Finalize();
        return;
    }
    string str_inputFileName = inputFileName.toStdString();
    const char *s_inputFileName = str_inputFileName.c_str();
    PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString((char *)s_inputFileName));
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);

    PyObject *ob1, *ob2;
    if (PyTuple_Check(pValue))
    {
        PyArg_ParseTuple(pValue, "O|O:ref", &ob1, &ob2); // o-> pyobject |i-> int|s-> char*
    }
    else
    {
        emit QmessageSend(tr("Error"), tr("<p> No nonogram detect</p>"));
        return;
    }
    vector<vector<int>> rowList;
    vector<vector<int>> colList;

    TwoDlistTupleToTwoDVector_Int(ob1, rowList);
    TwoDlistTupleToTwoDVector_Int(ob2, colList);

    // Py_Finalize();
#endif /*ML_USING_CPLUSPLUS_EN*/

    emit detectOutputSend(rowList, colList);
    return;
}

#if !ML_USING_CPLUSPLUS_EN
// ref: https://gist.github.com/rjzak/5681680
// PyObject -> Vector
vector<int> imagedetector::listTupleToVector_Int(PyObject *incoming)
{
    vector<int> data;
    if (PyTuple_Check(incoming))
    {
        for (Py_ssize_t i = 0; i < PyTuple_Size(incoming); i++)
        {
            PyObject *value = PyTuple_GetItem(incoming, i);
            data.push_back(PyFloat_AsDouble(value));
        }
    }
    else
    {
        if (PyList_Check(incoming))
        {
            for (Py_ssize_t i = 0; i < PyList_Size(incoming); i++)
            {
                PyObject *value = PyList_GetItem(incoming, i);
                data.push_back(PyFloat_AsDouble(value));
            }
        }
        else
        {
            emit QmessageSend(tr("Error"), tr("<p> Passed PyObject pointer was not a list or tuple!</p>"));
        }
    }
    return data;
}

void imagedetector::TwoDlistTupleToTwoDVector_Int(PyObject *incoming, vector<vector<int>> &res)
{
    if (PyTuple_Check(incoming))
    {
        for (Py_ssize_t i = 0; i < PyTuple_Size(incoming); i++)
        {
            PyObject *value = PyTuple_GetItem(incoming, i);
            res.push_back(listTupleToVector_Int(value));
        }
    }
    else
    {
        if (PyList_Check(incoming))
        {
            for (Py_ssize_t i = 0; i < PyList_Size(incoming); i++)
            {
                PyObject *value = PyList_GetItem(incoming, i);
                res.push_back(listTupleToVector_Int(value));
            }
        }
        else
        {
            emit QmessageSend(tr("Error"), tr("<p> Passed PyObject pointer was not a list or tuple!</p>"));
        }
    }
}
#endif /*!ML_USING_CPLUSPLUS_EN*/