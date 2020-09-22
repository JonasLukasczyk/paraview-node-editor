#include <Port.h>

// qt includes
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QApplication>
#include <QPalette>
#include <QPen>

NE::Port::Port(
        int type,
        QString name,
        QGraphicsItem* parent
    )
    : QGraphicsItem(parent)
{
    if(name!=""){
        this->label = new QGraphicsTextItem("", this);
        this->label->setHtml("<h4 align='"+QString(type==0 ? "left" : "right")+"'>"+name+"</h4>");
        this->label->setCursor( Qt::PointingHandCursor );
        this->label->setPos(
            type==0
                ? this->portRadius+3
                : -this->portRadius-3 - this->label->boundingRect().width(),
            -0.5*this->label->boundingRect().height()
        );
    }

    this->disc = new QGraphicsEllipseItem(
        -this->portRadius,
        -this->portRadius,
        2*this->portRadius,
        2*this->portRadius,
        this
    );
    this->disc->setBrush( QApplication::palette().dark() );
    this->setStyle( 0 );
}

NE::Port::~Port(){}

int NE::Port::setStyle(int style){
    this->disc->setPen(
        QPen(
            style==1
                ? QApplication::palette().highlight()
                : QApplication::palette().light(),
            this->borderWidth
        )
    );
    return 1;
}

QRectF NE::Port::boundingRect() const {
    return QRectF(0,0,0,0);
}

void NE::Port::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
}