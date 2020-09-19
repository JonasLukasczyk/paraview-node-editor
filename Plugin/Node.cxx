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
#include <pqProxyWidget.h>

#include <pqProxiesWidget.h>
#include <pqServerManagerModel.h>
#include <pqView.h>
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
        ResizeInterceptor(Node* n) : QObject(n){
            this->node = n;
        }
        bool eventFilter(QObject *object, QEvent *event){
            if(event->type()==QEvent::LayoutRequest)
                this->node->updateSize();
            return false;
        }
};

// TODO: find better way to do this
class ClickInterceptor : public QObject {
    public:
        Node* node;
        ClickInterceptor(Node* n) : QObject(n){
            this->node = n;
        }
        bool eventFilter(QObject *object, QEvent *event){
            if(
                event->type()==QEvent::GraphicsSceneMousePress
                && ((QGraphicsSceneMouseEvent*) event)->button()==2
            )
                this->node->advanceVerbosity();
            return false;
        }
};

int Node::advanceVerbosity(){
    this->verbosity++;
    if(this->verbosity>2)
        this->verbosity=0;

    if(this->verbosity==0)
        this->proxyProperties->filterWidgets(false, "%%%%%%%%%%%%%%");
    else if(this->verbosity==1)
        this->proxyProperties->filterWidgets(false);
    else
        this->proxyProperties->filterWidgets(true);

    return 1;
}

// TODO: this variable is currently used to position nodes by creation in a line
// In the future this will be replaced with a layout algorithm
int todoXOffset = -400;

Node::Node(pqProxy* proxy, QGraphicsItem *parent) :
    QObject(),
    QGraphicsItem(parent),
    proxy(proxy)
{
    std::cout<<"Creating Node: "<< this->print() <<std::endl;

    // set options
    this->setFlag(ItemIsMovable);
    this->setFlag(ItemSendsGeometryChanges);
    this->setCacheMode(DeviceCoordinateCache);
    this->setZValue(1);

    // set initial position
    this->setPos(
        todoXOffset+=350,
        0
    );

    // determine port height
    auto proxyAsView = dynamic_cast<pqView*>(proxy);
    auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy);
    auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

    if(proxyAsView){
        this->portContainerHeight = this->portHeight; // room for one port
    } else if (proxyAsFilter){
        this->portContainerHeight = std::max(
            proxyAsFilter->getNumberOfInputPorts(),
            proxyAsFilter->getNumberOfOutputPorts()
        )*this->portHeight;
    } else if (proxyAsSource){
        this->portContainerHeight = proxyAsSource->getNumberOfOutputPorts()*this->portHeight;
    }

    // create a widget container for property and display widgets
    {
        this->widgetContainer = new QWidget;
        this->widgetContainer->setMinimumWidth(this->width);
        this->widgetContainer->setMaximumWidth(this->width);

        // install resize event filter
        this->widgetContainer->installEventFilter(
            new ResizeInterceptor(this)
        );
    }

    // init label
    {
        auto labelItem = new QGraphicsTextItem("",this);
        labelItem->setPos(
            0,
            -this->portContainerHeight - this->labelHeight
        );
        labelItem->setTextWidth(this->width);
        labelItem->setHtml("<h2 align='center'>"+this->proxy->getSMName()+"</h2>");

        QObject::connect(
            proxy, &pqPipelineSource::nameChanged,
            this, [=](){ labelItem->setHtml("<h2 align='center'>"+proxy->getSMName()+"</h2>"); }
        );

        labelItem->installEventFilter(
            new ClickInterceptor(this)
        );
    }

    // initialize property widgets container
    {
        auto containerLayout = new QVBoxLayout;
        this->widgetContainer->setLayout(containerLayout);

        auto graphicsProxyWidget = new QGraphicsProxyWidget(this);
        graphicsProxyWidget->setWidget( this->widgetContainer );
        graphicsProxyWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

        this->proxyProperties = new pqProxyWidget(this->proxy->getProxy());
        this->proxyProperties->updatePanel();
        containerLayout->addWidget(this->proxyProperties);

        this->advanceVerbosity();
    }
}

Node::Node(pqPipelineSource* proxy, QGraphicsItem *parent) :
    Node((pqProxy*)proxy, parent)
{
    // create ports
    {
        if(auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy)){
            for(int i=0; i<proxyAsFilter->getNumberOfInputPorts(); i++){
                this->addPort(
                    0,
                    i,
                    proxyAsFilter->getInputPortName(i)
                );
            }
        }

        for(int i=0; i<proxy->getNumberOfOutputPorts(); i++){
            this->addPort(
                1,
                i,
                proxy->getOutputPort(i)->getPortName()
            );
        }
    }

    // create property widgets
    QObject::connect(
        this->proxyProperties, &pqProxyWidget::changeFinished,
        this, [=](){
            std::cout<<"Property Modified: "<<this->print()<<std::endl;
            this->proxy->setModifiedState(pqProxy::MODIFIED);
            this->proxyProperties->apply();
        }
    );
}

Node::Node(pqView* proxy, QGraphicsItem *parent) :
    Node((pqProxy*)proxy, parent)
{
    // create port
    this->addPort(
        2,
        0,
        "Elements"
    );

    // create property widgets
    QObject::connect(
        this->proxyProperties, &pqProxyWidget::changeFinished,
        [=](){
            std::cout<<"Property Modified: "<<this->print()<<std::endl;
            this->proxy->setModifiedState(pqProxy::MODIFIED);
            this->proxyProperties->apply();
            ((pqView*)this->proxy)->render();
        }
    );
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent * event){
    QGraphicsItem::mousePressEvent(event);
    emit nodeClicked();
}

int Node::addPort(int type, const int index, const QString& portLabel){

    qreal x = type==0
        ? -this->padding
        : type==1
            ? this->width + this->padding
            : this->width*0.5;
    qreal y = type==2
        ? -this->portContainerHeight - this->labelHeight - this->padding
        : -this->portContainerHeight + (index+0.5)*this->portHeight;

    auto palette = QApplication::palette();
    QPen pen(palette.light(), this->borderWidth);

    if(type!=2){
        auto label = new QGraphicsTextItem("", this);
        label->setPos(
            type==0
                ? 2*this->padding
                : this->width*0.5,
            y-this->portRadius-5
        );
        label->setTextWidth(0.5*this->width - 2*this->padding);
        label->setHtml("<h4 align='"+QString(type==0 ? "left" : "right")+"'>"+portLabel+"</h4>");
    }

    auto port = new QGraphicsEllipseItem(
        -this->portRadius,
        -this->portRadius,
        2*this->portRadius,
        2*this->portRadius,
        this
    );
    port->setPos( x, y );
    port->setPen(pen);
    port->setBrush( palette.dark() );

    if(type==1)
        this->oPorts.push_back( port );
    else
        this->iPorts.push_back( port );

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

    emit this->nodeResized();

    return 1;
}

int Node::setType(int type){
    this->type = type;
    this->update(this->boundingRect());
    return this->type;
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
        this->type==0
            ? palette.light()
            : this->type==1
                ? palette.highlight()
                : QColor("#e9763d"),
        this->borderWidth
    );

    painter->setRenderHints(QPainter::HighQualityAntialiasing);
    painter->setPen(pen);
    painter->fillPath(path, palette.window());
    painter->drawPath(path);
}