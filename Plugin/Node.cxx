#include <Node.h>

// node editor includes
#include <Edge.h>

// qt includes
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QGraphicsEllipseItem>

// paraview/vtk includes
#include <pqProxiesWidget.h>
#include <pqServerManagerModel.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <vtkSMProxy.h>

// std includes
#include <iostream>
#include <sstream>

// Instead of subclassing the widgetContainer I install an event filter to detect
// resize events within the cotainer which triggers a resize of the entire node.
// TODO: find better way to do this
class ResizeInterceptor : public QObject {
    public:
        Node* node;
        ResizeInterceptor(Node* n) : QObject(n->getWidgetContainer()){
            this->node = n;
        }
        bool eventFilter(QObject *object, QEvent *event){
            if(event->type()==QEvent::LayoutRequest)
                this->node->updateSize();
            return false;
        }
};


// TODO: this variable is currently used to position nodes by creation in a line
// In the future this will be replaced with a layout algorithm
int todoXOffset = -400;

Node::Node(pqPipelineSource* source, QGraphicsItem *parent) :
    QObject(),
    QGraphicsItem(parent),
    proxy(source)
{
    std::cout<<"Creating Source/Filter Node: "<< this->print() <<std::endl;

    // set initial position
    this->setPos(
        todoXOffset+=400,
        0
    );

    // set options
    this->setFlag(ItemIsMovable);
    this->setFlag(ItemSendsGeometryChanges);
    this->setCacheMode(DeviceCoordinateCache);
    this->setZValue(1);

    // create a widget container for property and display widgets
    this->widgetContainer = new QWidget;
    this->widgetContainer->setMinimumWidth(this->width);
    this->widgetContainer->setMaximumWidth(this->width);

    // install resize event filter
    this->widgetContainer->installEventFilter(
        new ResizeInterceptor(this)
    );

    // determine number of input and output ports
    int nOutputPorts = source->getNumberOfOutputPorts();
    int nInputPorts = 0;
    auto sourceAsFilter = dynamic_cast<pqPipelineFilter*>(this->proxy);
    if(sourceAsFilter){
        nInputPorts = sourceAsFilter->getNumberOfInputPorts();
    }
    this->portContainerHeight = std::max(nOutputPorts,nInputPorts)*this->portHeight;

    this->initLabel();

    // create ports
    {
        for(int i=0; i<nInputPorts; i++){
            this->addPort(
                true,
                i,
                sourceAsFilter->getInputPortName(i)
            );
        }

        for(int i=0; i<nOutputPorts; i++){
            this->addPort(
                false,
                i,
                source->getOutputPort(i)->getPortName()
            );
        }
    }

    // create property widgets (in the future also display widgets)
    {
        auto proxiesWidget = new pqProxiesWidget;
        proxiesWidget->addProxy( source->getProxy(), "Properties" );
        proxiesWidget->updateLayout();
        proxiesWidget->filterWidgets(true);

        auto containerLayout = new QVBoxLayout;
        containerLayout->addWidget(proxiesWidget);

        QObject::connect(
            proxiesWidget, &pqProxiesWidget::changeFinished,
            [=](){
                std::cout<<"Property Modified: "<<this->print()<<std::endl;
                this->proxy->setModifiedState(pqProxy::MODIFIED);
            }
        );

        this->widgetContainer->setLayout(containerLayout);
    }

    // embed widget container in node
    {
        auto graphicsProxyWidget = new QGraphicsProxyWidget(this);
        graphicsProxyWidget->setWidget( this->widgetContainer );
        graphicsProxyWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    }
}

int Node::initLabel(){
    auto labelItem = new QGraphicsTextItem("", this);
    labelItem->setPos(
        0,
        -this->portContainerHeight - this->labelHeight
    );
    labelItem->setTextWidth(this->width);
    labelItem->setHtml("<h2 align='center'>"+this->proxy->getSMName()+"</h2>");

    QObject::connect(
        this->proxy, &pqPipelineSource::nameChanged,
        [=](){ labelItem->setHtml("<h2 align='center'>"+this->proxy->getSMName()+"</h2>"); }
    );

    return 1;
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent * event){
    QGraphicsItem::mousePressEvent(event);
    emit nodeClicked();
}

int Node::addPort(bool isInputPort, const int index, const QString& portLabel){

    qreal x = isInputPort
        ? -this->padding
        : this->width + this->padding;
    qreal y = -this->portContainerHeight + (index+0.5)*this->portHeight;

    auto palette = QApplication::palette();
    QPen pen(palette.light(), this->borderWidth);

    auto label = new QGraphicsTextItem("", this);
    label->setPos(
        isInputPort
            ? 2*this->padding
            : this->width*0.5,
        y-this->portRadius-3
    );
    label->setTextWidth(0.5*this->width - 2*this->padding);
    // label->setHtml("<h4 align='right'>"+port->getPortName()+"&lt;"+port->getDataClassName()+"&gt;</h4>");
    label->setHtml("<h4 align='"+QString(isInputPort ? "left" : "right")+"'>"+portLabel+"</h4>");

    auto port = new QGraphicsEllipseItem(
        -this->portRadius,
        -this->portRadius,
        2*this->portRadius,
        2*this->portRadius,
        this
    );
    port->setPos(
        x,
        y
    );
    port->setPen(pen);
    port->setBrush( palette.dark() );

    if(isInputPort)
        this->iPorts.push_back( port );
    else
        this->oPorts.push_back( port );

    return 1;
}

Node::~Node(){
    std::cout<<"Deleting Node: "<< this->print() <<std::endl;
    delete this->widgetContainer;
}

int Node::updateSize(){
    this->widgetContainer->resize(
        this->widgetContainer->layout()->sizeHint()
    );
    this->prepareGeometryChange();

    return 1;
}

int Node::setState(int state){
    this->state = state;

    this->update(this->boundingRect());

    return this->state;
}

std::string Node::print(){
    std::stringstream ss;
    ss
        <<this->proxy->getSMName().toStdString()
        <<"<"<<this->proxy->getProxy()->GetGlobalID()<<">"
    ;
    return ss.str();
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value){
    switch (change) {
        case ItemPositionHasChanged:
            emit this->nodeMoved();
            break;
        default:
            break;
    };

    return QGraphicsItem::itemChange(change, value);
}

QRectF Node::boundingRect() const {
    auto offset = this->borderWidth+this->padding;

    return QRectF(
        -offset,
        -offset - this->portContainerHeight - this->labelHeight,
        this->widgetContainer->width()  + 2*offset,
        this->widgetContainer->height() + 2*offset + this->portContainerHeight + this->labelHeight
    );
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    auto palette = QApplication::palette();

    QPainterPath path;
    path.addRoundedRect(
        QRect(
            -this->padding,
            -this->padding - this->portContainerHeight - this->labelHeight,
            this->widgetContainer->width() + 2*this->padding,
            this->widgetContainer->height() + 2*this->padding + this->portContainerHeight + this->labelHeight
        ),
        10, 10
    );
    QPen pen(
        this->state
            ? palette.highlight()
            : palette.light(),
        this->borderWidth
    );

    painter->setRenderHints(QPainter::HighQualityAntialiasing);
    painter->setPen(pen);
    painter->fillPath(path, palette.window());
    painter->drawPath(path);
}