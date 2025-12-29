# Game Programming 25
Exam Repository for our Game in the Game Programming Course 2025.

## Students
| Name | ITU Mail |
| :--- | :--- |
| **Adam Hadou Temsamani** | ahad@itu.dk |
| **Maxime Birkkjær Havez** | biha@itu.dk |

## Setup
There two ways of compiling, and running our project:
### VSCode
1. download and install [CMake](https://cmake.org/download/) (for mac users, use [brew](https://brew.sh/) instead of downloading from the website)
2. download [VSCode](https://code.visualstudio.com/download) and install these two extensions (both of them from Microsoft)
    1. C/C++ Extension Pack 
    2. CMake Tools
3. clone the repository `git clone --recurse-submodules https://github.itu.dk/biha/gp_25_exam_project.git` 
4. open repository in VSCode
5. in `Preferences->Settings`, search for `cmake path` and replace the content with the path to your CMake executable (you can find in typing `where cmake` or `which cmake` on the command line)
6. restart the editor
After reopening the editor, you should see all available targets in the cmake tab, in the `Project Outline` section.

Build and run them from there, or set one to be the "default" target (`right-click->Set Launch/Debug Target`)

### CLI (Prefered)
This setup assumes you have `Cmake` and your prefered `C++` compiler of choise installed
1. clone the repository `git clone --recurse-submodules https://github.itu.dk/biha/gp_25_exam_project.git` 
2. create the build directory `mkdir out`
3. configure the project `cmake . -B out`
4. build the main executable target `cmake --build out --target main.cpp`
5. inside the `/out` directory, you should find an executable `main.cpp.exe` (if on windows; otherwise should have a similar name)

