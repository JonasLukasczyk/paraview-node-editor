#include <Node.h>

// node editor includes
#include <Utils.h>
#include <Port.h>

// qt includes
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsProxyWidget>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>

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

NE::Node::Node(QGraphicsScene* scene, pqProxy* proxy, QGraphicsItem *parent) :
    QObject(),
    QGraphicsItem(parent),
    scene(scene),
    proxy(proxy)
{
    NE::log("  +Node: " + NE::getLabel(proxy));

    // set options
    this->setFlag(ItemIsMovable);
    this->setFlag(ItemSendsGeometryChanges);
    this->setCacheMode(DeviceCoordinateCache);
    this->setCursor(Qt::ArrowCursor);
    this->setZValue(1);

    // determine port height
    auto proxyAsView = dynamic_cast<pqView*>(proxy);
    auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy);
    auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

    if (proxyAsFilter){
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
        this->widgetContainer->setMinimumWidth(NE::CONSTS::NODE_WIDTH);
        this->widgetContainer->setMaximumWidth(NE::CONSTS::NODE_WIDTH);

        // install resize event filter
        this->widgetContainer->installEventFilter(
            NE::createInterceptor(
                this->widgetContainer,
                [=](QObject* object, QEvent* event){
                    if(event->type()==QEvent::LayoutRequest)
                        this->updateSize();
                    return false;
                }
            )
        );
    }

    // init label
    {
        this->label = new QGraphicsTextItem("",this);
        this->label->setCursor( Qt::PointingHandCursor );

        QFont font;
        font.setBold(true);
        font.setPointSize(13);
        this->label->setFont(font);

        auto nameChangeFunc = [=](){
            this->label->setPlainText(proxy->getSMName());
            auto br = this->label->boundingRect();
            this->label->setPos(
                0.5*(NE::CONSTS::NODE_WIDTH-br.width()),
                -this->portContainerHeight - this->labelHeight
            );
        };

        QObject::connect(
            proxy, &pqPipelineSource::nameChanged,
            this->label, nameChangeFunc
        );

        nameChangeFunc();
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

        this->updateSize();
    }

    this->scene->addItem(this);
}

NE::Node::Node(QGraphicsScene* scene, pqPipelineSource* proxy, QGraphicsItem *parent) :
    Node(scene, (pqProxy*)proxy, parent)
{
    // create ports
    {
        auto br = this->boundingRect();
        auto adjust = 0.5*NE::CONSTS::NODE_BORDER_WIDTH + NE::CONSTS::NODE_BR_PADDING;
        br.adjust(adjust,adjust,-adjust,-adjust);

        if(auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy)){
            for(int i=0; i<proxyAsFilter->getNumberOfInputPorts(); i++){
                auto iPort = new Port(0,proxyAsFilter->getInputPortName(i),this);
                iPort->setPos(
                    br.left(),
                    -this->portContainerHeight + (i+0.5)*this->portHeight
                );
                this->iPorts.push_back( iPort );
            }
        }

        for(int i=0; i<proxy->getNumberOfOutputPorts(); i++){
            auto oPort = new Port(1,proxy->getOutputPort(i)->getPortName(),this);
            oPort->setPos(
                br.right(),
                -this->portContainerHeight + (i+0.5)*this->portHeight
            );
            this->oPorts.push_back( oPort );
        }
    }

    // create property widgets
    QObject::connect(
        this->proxyProperties, &pqProxyWidget::changeFinished,
        this, [=](){
            NE::log("Source/Filter Property Modified: "+NE::getLabel(this->proxy));
            this->proxy->setModifiedState(pqProxy::MODIFIED);
            return 1;
        }
    );
    QObject::connect(
        this->proxy, &pqProxy::modifiedStateChanged,
        this, [=](){
            if(this->proxy->modifiedState()==pqProxy::ModifiedState::MODIFIED)
                this->setBackgroundStyle(1);
            else
                this->setBackgroundStyle(0);
            return 1;
        }
    );
}

NE::Node::Node(QGraphicsScene* scene, pqView* proxy, QGraphicsItem *parent) :
    Node(scene, (pqProxy*)proxy, parent)
{
    auto br = this->boundingRect();
    auto adjust = 0.5*NE::CONSTS::NODE_BORDER_WIDTH + NE::CONSTS::NODE_BR_PADDING;
    br.adjust(adjust,adjust,-adjust,-adjust);

    // create port
    auto iPort = new Port(2,"",this);
    iPort->setPos(
        br.center().x(),
        br.top()
    );
    this->iPorts.push_back( iPort );

    // create property widgets
    QObject::connect(
        this->proxyProperties, &pqProxyWidget::changeFinished,
        this, [=](){
            NE::log("View Property Modified: "+NE::getLabel(this->proxy));
            this->proxy->setModifiedState(pqProxy::MODIFIED);
            this->proxyProperties->apply();
            ((pqView*)this->proxy)->render();
        }
    );
}

NE::Node::~Node(){
    NE::log(" -Node: "+NE::getLabel(this->proxy));
    this->scene->removeItem(this);
}

int NE::Node::updateSize(){
    this->widgetContainer->resize(
        this->widgetContainer->layout()->sizeHint()
    );

    this->prepareGeometryChange();

    this->widgetContainerWidth = this->widgetContainer->width();
    this->widgetContainerHeight = this->widgetContainer->height();

    emit this->nodeResized();

    return 1;
}

int NE::Node::setOutlineStyle(int style){
    this->outlineStyle = style;
    this->update(this->boundingRect());
    return 1;
}
int NE::Node::setBackgroundStyle(int style){
    this->backgroundStyle = style;
    this->update(this->boundingRect());
    return 1;
}

int NE::Node::advanceVerbosity(){
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

QVariant NE::Node::itemChange(GraphicsItemChange change, const QVariant &value){
    switch (change) {
        case ItemPositionHasChanged:
            emit this->nodeMoved();
            break;
        default:
            break;
    };

    return QGraphicsItem::itemChange(change, value);
}

QRectF NE::Node::boundingRect() const {
    auto offset = NE::CONSTS::NODE_BORDER_WIDTH+NE::CONSTS::NODE_PADDING;
    auto br = QRectF(
        -offset,
        -offset - this->portContainerHeight - this->labelHeight,
        this->widgetContainerWidth  + 2*offset,
        this->widgetContainerHeight + 2*offset + this->portContainerHeight + this->labelHeight
    );
    br.adjust(
        -NE::CONSTS::NODE_BR_PADDING,
        -NE::CONSTS::NODE_BR_PADDING,
        NE::CONSTS::NODE_BR_PADDING,
        NE::CONSTS::NODE_BR_PADDING
    );

    return br;
}

void NE::Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    auto palette = QApplication::palette();

    QPainterPath path;
    int offset = 0.5*NE::CONSTS::NODE_BORDER_WIDTH + NE::CONSTS::NODE_BR_PADDING;
    auto br = this->boundingRect();
    br.adjust(offset,offset,-offset,-offset);
    path.addRoundedRect(br, 10, 10);

    QPen pen(
        this->outlineStyle==0
            ? palette.light()
            : this->outlineStyle==1
                ? palette.highlight()
                : NE::CONSTS::COLOR_ORANGE,
        NE::CONSTS::NODE_BORDER_WIDTH
    );

    painter->setPen(pen);
    painter->fillPath(path,
        this->backgroundStyle==1
            ? NE::CONSTS::COLOR_GREEN
            : palette.window()
    );
    painter->drawPath(path);
}