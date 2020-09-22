#include <Utils.h>

#include <pqProxy.h>
#include <vtkSMProxy.h>

#include <iostream>
#include <QColor>

bool   NE::CONSTS::DEBUG = false;
int    NE::CONSTS::NODE_WIDTH = 300;
int    NE::CONSTS::NODE_PADDING = 0;
int    NE::CONSTS::NODE_BORDER_WIDTH = 4;
int    NE::CONSTS::EDGE_WIDTH = 5;
QColor NE::CONSTS::COLOR_ORANGE = QColor("#e9763d");
QColor NE::CONSTS::COLOR_GREEN  = QColor("#049a0a");

void NE::log(std::string content, bool force){
    if(!force && !NE::CONSTS::DEBUG)
        return;

    std::cout<<content<<std::endl;
}

int NE::getID(pqProxy* proxy){
    return proxy->getProxy()->GetGlobalID();
}

std::string NE::getLabel(pqProxy* proxy){
    return proxy->getSMName().toStdString() + "<" + std::to_string(NE::getID(proxy)) + ">";
}

