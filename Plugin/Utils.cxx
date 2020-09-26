#include <Utils.h>

#include <pqProxy.h>
#include <vtkSMProxy.h>

#include <iostream>
#include <QColor>

#ifdef _WIN32
    #include <windows.h>
    #include <ciso646>
    #include <cwchar>
    #include <direct.h>
    #include <stdint.h>
    #include <time.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

bool   NE::CONSTS::DEBUG = true;
int    NE::CONSTS::NODE_WIDTH = 300;
int    NE::CONSTS::NODE_PADDING = 0;
int    NE::CONSTS::NODE_BORDER_WIDTH = 4;
int    NE::CONSTS::NODE_BR_PADDING = 0;
int    NE::CONSTS::EDGE_WIDTH = 5;
QColor NE::CONSTS::COLOR_ORANGE = QColor("#e9763d");
QColor NE::CONSTS::COLOR_GREEN  = QColor("#049a0a");
double NE::CONSTS::DOUBLE_CLICK_DELAY = 0.3;

void NE::log(std::string content, bool force){
    if(!force && !NE::CONSTS::DEBUG)
        return;

    std::cout<<content<<std::endl;
};

int NE::getID(pqProxy* proxy){
    return proxy
        ? proxy->getProxy()->GetGlobalID()
        : -1;
};

std::string NE::getLabel(pqProxy* proxy){
    return proxy
        ? proxy->getSMName().toStdString() + "<" + std::to_string(NE::getID(proxy)) + ">"
        : "PROXY IS NULL";
};

double NE::getTimeStamp(){
    #ifdef _WIN32
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        LARGE_INTEGER temp;
        QueryPerformanceCounter(&temp);

        return (double)temp.QuadPart / frequency.QuadPart;
    #endif

    #ifdef __APPLE__
        struct timeval stamp;
        gettimeofday(&stamp, NULL);
        return (stamp.tv_sec * 1000000 + stamp.tv_usec) / 1000000.0;
    #endif

    #ifdef __unix__
        struct timeval stamp;
        gettimeofday(&stamp, NULL);
        return (stamp.tv_sec * 1000000 + stamp.tv_usec) / 1000000.0;
    #endif
};

double t0{0};
double NE::getTimeDelta(){
    double t1 = NE::getTimeStamp();
    double delta = t1-t0;
    t0 = t1;
    return delta;
};

bool NE::isDoubleClick(){
    return NE::getTimeDelta()<NE::CONSTS::DOUBLE_CLICK_DELAY;
};
