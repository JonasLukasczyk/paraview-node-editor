#include <Utils.h>

#include <iostream>
#include <QColor>

bool   NE::CONSTS::DEBUG = true;
int    NE::CONSTS::NODE_WIDTH = 300;
int    NE::CONSTS::NODE_PADDING = 0;
int    NE::CONSTS::NODE_BORDER_WIDTH = 4;
int    NE::CONSTS::EDGE_WIDTH = 5;
QColor NE::CONSTS::COLOR_ORANGE = QColor("#e9763d");
QColor NE::CONSTS::COLOR_GREEN  = QColor("#049a0a");

void NE::log(std::string content){
    if(!NE::CONSTS::DEBUG)
        return;

    std::cout<<content<<std::endl;
}

