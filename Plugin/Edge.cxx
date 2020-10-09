#include <Edge.h>

// node editor includes
#include <Utils.h>
#include <Node.h>
#include <Port.h>
#include <Scene.h>

// paraview/vtk includes
#include <pqPipelineSource.h>
#include <vtkSMProxy.h>

// qt includes
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>
#include <QApplication>

// std includes
#include <sstream>

NE::Edge::Edge(
    QGraphicsScene* scene,
    Node* producer,
    int producerOutputPortIdx,
    Node* consumer,
    int consumerInputPortIdx,
    int type,
    QGraphicsItem *parent
) :
    QObject(),
    QGraphicsPathItem(parent),
    scene(scene),
    producer(producer),
    producerOutputPortIdx(producerOutputPortIdx),
    consumer(consumer),
    consumerInputPortIdx(consumerInputPortIdx),
    type(type)
{
    NE::log("  +Edge: "+ this->toString());

    this->connect(
        this->producer, &Node::nodeMoved,
        this, &Edge::updatePoints
    );
    this->connect(
        this->consumer, &Node::nodeMoved,
        this, &Edge::updatePoints
    );
    this->connect(
        this->producer, &Node::nodeResized,
        this, &Edge::updatePoints
    );
    this->connect(
        this->consumer, &Node::nodeResized,
        this, &Edge::updatePoints
    );

    this->setAcceptedMouseButtons(Qt::NoButton);
    this->setZValue(type>0 ? 3 : 2);

    this->updatePoints();

    this->scene->addItem(this);
}

NE::Edge::~Edge() {
    NE::log("  -Edge: " + this->toString());
    this->scene->removeItem(this);
}

int NE::Edge::setType(int type){
    this->type = type;
    this->update(this->boundingRect());
    return this->type;
}

std::string NE::Edge::toString(){
    std::stringstream ss;

    ss
        <<NE::getLabel(this->producer->getProxy())
        <<"["<<this->producerOutputPortIdx<<"]"
        <<" -> "
        <<NE::getLabel(this->consumer->getProxy())
        <<"["<<this->consumerInputPortIdx<<"]"
    ;
    return ss.str();
}

QRectF NE::Edge::boundingRect() const {
    qreal x0 = std::min(this->oPoint.x(),std::min(this->cPoint.x(), this->iPoint.x()));
    qreal y0 = std::min(this->oPoint.y(),std::min(this->cPoint.y(), this->iPoint.y()));
    qreal x1 = std::max(this->oPoint.x(),std::max(this->cPoint.x(), this->iPoint.x()));
    qreal y1 = std::max(this->oPoint.y(),std::max(this->cPoint.y(), this->iPoint.y()));

    const qreal extra = 4.0;
    return QRectF( x0,y0,x1-x0,y1-y0 )
        .adjusted(-extra, -extra, extra, extra);
}

int NE::Edge::updatePoints(){

    auto nProducerOutputPorts = this->producer->getOutputPorts().size();
    auto b = this->producer->boundingRect();

    this->prepareGeometryChange();

    this->oPoint = QGraphicsItem::mapFromItem(this->producer->getOutputPorts()[this->producerOutputPortIdx]->getDisc(), 0, 0);
    this->iPoint = QGraphicsItem::mapFromItem(this->consumer->getInputPorts()[this->consumerInputPortIdx]->getDisc(), 0, 0);

    this->cPoint = this->type==0
        ? this->oPoint
        : this->mapFromItem(
            this->producer,
            b.bottomRight()) + QPointF((nProducerOutputPorts-this->producerOutputPortIdx)*15,
            0
        );

    return 1;
}

void NE::Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    QLineF line(this->oPoint, this->iPoint);

    QPainterPath path;
    path.moveTo(this->oPoint);

    if(this->type==0){
        path.lineTo(this->iPoint);
    } else {
        path.quadTo(
            this->cPoint.x(), this->oPoint.y(),
            this->cPoint.x(), this->oPoint.y()+40
        );
        path.lineTo(this->cPoint);
        auto xt = 0.5*(this->cPoint.x()-this->iPoint.x());
        auto yt = 0.5*(this->iPoint.y()-this->cPoint.y());
        path.cubicTo(
            this->cPoint.x(), this->cPoint.y()+yt,
            this->iPoint.x()+xt, this->cPoint.y()+yt,
            this->iPoint.x(), this->iPoint.y()
        );
    }

    painter->setPen(
        QPen(
            this->type==0
                ? QApplication::palette().highlight().color()
                : NE::CONSTS::COLOR_ORANGE,
            NE::CONSTS::EDGE_WIDTH,
            this->type==0
                ? Qt::SolidLine
                : Qt::DashDotLine,
            Qt::RoundCap,
            Qt::RoundJoin
        )
    );
    painter->drawPath(path);
}