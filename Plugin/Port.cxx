#include <Port.h>

#include <Utils.h>

// qt includes
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QApplication>
#include <QPalette>
#include <QPen>
#include <QFont>

#include <iostream>

class PortLabel : public QGraphicsTextItem {
    public:
    PortLabel(QString label, QGraphicsItem* parent):QGraphicsTextItem(label,parent){
        this->setCursor( Qt::PointingHandCursor );
    };
    ~PortLabel(){
    }
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override{
        auto font = this->font();
        font.setBold(true);
        this->setFont(font);
    };
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override{
        auto font = this->font();
        font.setBold(false);
        this->setFont(font);
    };
};

NE::Port::Port(
        int type,
        QString name,
        QGraphicsItem* parent
    )
    : QGraphicsItem(parent)
{
    this->label = new PortLabel(name, this);
    this->label->setPos(
        type==0
            ? this->portRadius+3
            : -this->portRadius-3 - this->label->boundingRect().width(),
        -0.5*this->label->boundingRect().height()
    );

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

NE::Port::~Port(){
}

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