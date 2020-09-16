#include <Edge.h>

// node editor includes
#include <Node.h>
#include <NodeEditorScene.h>

// paraview/vtk includes
#include <pqPipelineSource.h>
#include <vtkSMProxy.h>

// qt includes
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QApplication>

// std includes
#include <iostream>
#include <sstream>

Edge::Edge(
    Node* producer,
    int producerOutputPortIdx,
    Node* consumer,
    int consumerInputPortIdx,
    QGraphicsItem *parent
) :
    QObject(),
    QGraphicsPathItem(parent),
    producer(producer),
    producerOutputPortIdx(producerOutputPortIdx),
    consumer(consumer),
    consumerInputPortIdx(consumerInputPortIdx)
{
    std::cout << "Creating Edge: " << this->print() << std::endl;

    this->edgePen = new QPen(QApplication::palette().highlight(), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    QObject::connect(
        this->producer, &Node::nodeMoved,
        this, &Edge::updateEndPoints
    );
    QObject::connect(
        this->consumer, &Node::nodeMoved,
        this, &Edge::updateEndPoints
    );

    this->setZValue(0);

    this->updateEndPoints();
}

Edge::~Edge() {
    std::cout << "Deleting Edge: " << this->print() << std::endl;
    delete this->edgePen;
}

std::string Edge::print(){
    std::stringstream ss;
    ss
        <<this->producer->getProxy()->getSMName().toStdString()
        <<"<"<<this->producer->getProxy()->getProxy()->GetGlobalID()<<">"
        <<"["<<this->producerOutputPortIdx<<"]"
        <<" -> "
        <<this->consumer->getProxy()->getSMName().toStdString()
        <<"<"<<this->consumer->getProxy()->getProxy()->GetGlobalID()<<">"
        <<"["<<this->consumerInputPortIdx<<"]"
    ;
    return ss.str();
}

QRectF Edge::boundingRect() const {
    qreal x0 = this->oPoint.x() < this->iPoint.x() ? this->oPoint.x() : this->iPoint.x();
    qreal x1 = this->oPoint.x() > this->iPoint.x() ? this->oPoint.x() : this->iPoint.x();
    qreal y0 = this->oPoint.y() < this->iPoint.y() ? this->oPoint.y() : this->iPoint.y();
    qreal y1 = this->oPoint.y() > this->iPoint.y() ? this->oPoint.y() : this->iPoint.y();

    const qreal extra = 10.0;
    return QRectF( x0,y0,x1-x0,y1-y0 )
        .adjusted(-extra, -extra, extra, extra);
}

void Edge::updateEndPoints(){
    QLineF line(
        mapFromItem(this->producer->getOutputPorts()[this->producerOutputPortIdx], 0, 0),
        mapFromItem(this->consumer->getInputPorts()[this->consumerInputPortIdx], 0, 0)
    );
    qreal length = line.length();

    this->prepareGeometryChange();

    this->oPoint = line.p1();
    this->iPoint = line.p2();
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    painter->setRenderHints(QPainter::HighQualityAntialiasing);
    QLineF line(this->oPoint, this->iPoint);
    painter->setPen(*this->edgePen);
    painter->drawLine(line);
}