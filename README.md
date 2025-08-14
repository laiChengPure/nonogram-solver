# Qt project - nonogram


## Description
This is an application that can automatically solve nonograms.  
User can refer to the following link for nonogram rules:https://activityworkshop.net/puzzlesgames/nonograms/tutorial.html  
Users have the following input options:  
**1.First, enter the desired number of rows and columns for the nonogram. The application will then generate a corresponding grid. Users can then input the hint numbers for each row and column into the grid.**  
**2.Alternatively, users can directly select an existing nonogram puzzle image. The application will use machine learning to recognize the size of the nonogram and extract the hint numbers for each row and column.**  
Once the input is complete, press "Start Solving," and the application will begin solving the puzzle. After a short wait, the solution will be displayed along with the corresponding image.

## Prerequisites
**1. Download Qt**  
First, download Qt from https://www.qt.io/download-dev.  
**2. Add environment variables**  
After installation, add the necessary files to the system environment variables.  
For example, the qmake command is located in Qt\Tools\QtCreator\bin.  
If this step is skipped, user may encounter the following error:   
>'make' is not recognized as an internal or external command, operable program, or batch file.

**3. Download C++ 17**  
It seems that Qt6 requires C++17. Previously, I encountered an error when using Qt6 with a project that was originally built with Qt5. Running qmake followed by make resulted in the following error:  
>Qt qmake requires a C++17 compiler

**4. Download Python**  
Qt itself is written in C++, but the machine learning component is implemented in Python.  
**5. Install the necessary Python libraries**  
When calling Python from C++, the function   
*Py_Initialize();*   
is often used. However, compiling the project without proper setup may result in the following error:  
>undefined reference to `_imp__Py_Initialize'

This Stack Overflow post provides an explanation:  
https://stackoverflow.com/questions/78246210/problem-with-compiling-c-project-that-is-running-python-code-using-python-h  
> [!NOTE]
> You are attempting to link a program compiled with Windows g++ (x86_64-w64-mingw32-g++.exe) against an import library C:/Python310/libs/python310.lib that was built with Micosoft MSVC.
> That will not work because x86_64-w64-mingw32 libraries are incompatible with MSVC libraries. Get the x86_64-w64-mingw32 python library for MSYS2 and build using its header files and libraries.

In short, python310.lib was built using Microsoft MSVC, and the current compiler (g++) is incompatible with it.  
User can download the compatible Python library for MSYS2 from:  
[`x86_64-w64-mingw32` python library for MSYS2](https://packages.msys2.org/package/mingw-w64-x86_64-python)  
  
Steps to install MSYS2 and the necessary Python libraries:   
(1) Download MSYS2 from https://www.msys2.org/.  
(2) After installation, open ucrt64.exe.  
(3) In the terminal, run:  
pacman -S mingw-w64-ucrt-x86_64-gcc  
(This step may or may not be necessary.)  
(4) In the same terminal, run:  
pacman -S mingw-w64-x86_64-python   
(5) After installation, user should find the necessary Python header files and libraries in:  
*-C:\msys64\mingw64\include\python3.12*  
*-C:\msys64\mingw64\lib*

## Compilation Process
### 1. qmake -project "QT+=widgets”
This command generates a .pro project file using qmake.
Since Qt4 transitioned to Qt5, the QtWidgets module was separated from QtGui, and it is no longer included by default. Therefore, it must be manually added to the .pro file using QT+=widgets.    
> [!NOTE]
> Depending on the header files used in the project, additional modules may need to be added to the .pro file.    
> For example, if the project includes QMediaPlayer, the official documentation states that the multimedia module must be added.
> Otherwise, make will fail to recognize the QMediaPlayer class. The command should then be:  
> ### qmake -project "QT+=widgets" "QT+=multimedia" "QT += multimediawidgets”
### 2. qmake
This command generates a Makefile based on the .pro file.  
### 3. Modify Makefile.Debug and Makefile.Release  
After running qmake, two files will be generated: Makefile.Debug and Makefile.Release.  
Modify these files by adding the following lines:  
**In the INCPATH section, add:**  
> -I C:\msys64\mingw64\include -I C:\msys64\mingw64\include\python3.12

This ensures that the C++ compiler can locate the necessary Python headers (e.g., Python.h). The path depends on where MSYS2 was installed.  

**In the LIBS section, add:**  
> -L C:\RoboDK\Python37 -lpython37

This specifies the location of the Python interpreter. The path depends on where Python was installed.    
> [!NOTE]
> Do not set the path to -L C:\msys64\mingw64\lib -lpython3.12 (i.e., do not use the Python executable inside the MSYS2 directory).  
> The interpreter used for executing Python scripts should be the same one normally used in the command line.

### 4. make
This command compiles the project based on the Makefile and generates the executable file in the /release directory.
