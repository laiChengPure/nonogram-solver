// ref: https://blog.csdn.net/qq_36583051/article/details/108052297
// class about GIL interpreter & thread operation
#ifndef PYTHREADSTATELOCK_H
#define PYTHREADSTATELOCK_H
#include "Python.h"

class PyThreadStateLock
{
public:
	PyThreadStateLock(void)
	{
		_save = nullptr;
		nStatus = 0;
		nStatus = PyGILState_Check(); // detect whether the current thread own GIL
		PyGILState_STATE gstate;
		if (!nStatus)
		{
			gstate = PyGILState_Ensure(); // If it doesn't, apply for GIL
			nStatus = 1;
		}
		_save = PyEval_SaveThread();
		PyEval_RestoreThread(_save);
	}
	~PyThreadStateLock(void)
	{
		_save = PyEval_SaveThread();
		PyEval_RestoreThread(_save);
		if (nStatus)
		{
			PyGILState_Release(gstate); // current thread release GIL
		}
	}

private:
	PyGILState_STATE gstate;
	PyThreadState *_save;
	int nStatus;
};

#endif