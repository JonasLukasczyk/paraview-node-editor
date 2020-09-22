#include <NodeEditor.h>

// node editor includes
#include <Scene.h>
#include <View.h>
#include <Node.h>
#include <Edge.h>
#include <Port.h>
#include <Utils.h>

// qt includes
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QCheckBox>
#include <QEvent>
#include <QAction>
#include <iostream>

#include <QGraphicsSceneMouseEvent>

// paraview includes
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqActiveObjects.h>
#include <pqProxy.h>
#include <pqProxyWidget.h>
#include <pqProxySelection.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqView.h>

#include <vtkSMParaViewPipelineControllerWithRendering.h>

// std include
#include <iostream>

NodeEditor::NodeEditor(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    // create widget
    auto widget = new QWidget(this);

    // create layout
    auto layout = new QVBoxLayout;
    widget->setLayout(layout);

    // create node editor scene and view
    this->scene = new NE::Scene(this);
    this->view = new NE::View(
        this->scene,
        this
    );
    this->view->setDragMode( QGraphicsView::ScrollHandDrag );
    this->view->setSceneRect(-10000,-10000,20000,20000);
    layout->addWidget(this->view);

    this->initializeActions();
    this->createToolbar(layout);

    this->attachServerManagerListeners();

    this->setWidget(widget);
}

NodeEditor::NodeEditor(QWidget *parent) : NodeEditor("Node Editor", parent){
}

NodeEditor::~NodeEditor(){
}

int NodeEditor::apply(){
    auto nodes = this->nodeRegistry;
    for(auto it: nodes){
        auto proxy = dynamic_cast<pqPipelineSource*>(
            it.second->getProxy()
        );
        if(proxy){
            NE::log("Apply Properties: "+NE::getLabel(proxy));
            it.second->getProxyProperties()->apply();
            proxy->setModifiedState( pqProxy::ModifiedState::UNMODIFIED );
        }
    }
    for(auto it: nodes){
        auto proxy = dynamic_cast<pqPipelineSource*>(
            it.second->getProxy()
        );
        if(proxy){
            NE::log("Update Pipeline: "+NE::getLabel(proxy));
            proxy->updatePipeline();
        }
    }
    for(auto it: nodes){
        auto proxy = dynamic_cast<pqView*>(
            it.second->getProxy()
        );
        if(proxy){
            NE::log("Update View: "+NE::getLabel(proxy));
            proxy->render();
        }
    }

    return 1;
}

int NodeEditor::reset(){
    for(auto it: this->nodeRegistry){
        auto proxy = dynamic_cast<pqPipelineSource*>(
            it.second->getProxy()
        );
        if(proxy){
            NE::log("Reset Properties: "+NE::getLabel(proxy));
            it.second->getProxyProperties()->reset();
            proxy->setModifiedState( pqProxy::ModifiedState::UNMODIFIED );
        }
    }
    return 1;
}

int NodeEditor::zoom(){
    const int padding = 20;
    auto viewPort = this->scene->getBoundingRect(this->nodeRegistry);
    viewPort.adjust(-padding,-padding,padding,padding);
    this->view->fitInView(
        viewPort,
        Qt::KeepAspectRatio
    );
    return 1;
}

int NodeEditor::initializeActions(){
    this->actionApply = new QAction(this);
    QObject::connect(
        this->actionApply, &QAction::triggered,
        this, &NodeEditor::apply
    );

    this->actionReset = new QAction(this);
    QObject::connect(
        this->actionReset, &QAction::triggered,
        this, &NodeEditor::reset
    );

    this->actionZoom = new QAction(this);
    QObject::connect(
        this->actionZoom, &QAction::triggered,
        this, &NodeEditor::zoom
    );

    this->actionLayout = new QAction(this);
    QObject::connect(
        this->actionLayout, &QAction::triggered,
        this->scene, [=](){
            this->scene->computeLayout(
                this->nodeRegistry,
                this->edgeRegistry
            );
            return 1;
        }
    );

    this->actionAutoLayoutZoom = new QAction(this);
    QObject::connect(
        this->actionAutoLayoutZoom, &QAction::triggered,
        this->scene, [=](){
            if(this->autoUpdateLayout)
                this->actionLayout->trigger();
            if(this->autoUpdateZoom)
                this->actionZoom->trigger();
            return 1;
        }
    );

    return 1;
}

int NodeEditor::createToolbar(QLayout* layout){
    auto toolbar = new QWidget(this);
    layout->addWidget(toolbar);

    auto toolbarLayout = new QHBoxLayout;
    toolbar->setLayout(toolbarLayout);

    auto addButton = [=](QString label, QAction* action){
        auto button = new QPushButton(label);
        this->connect(
            button, &QPushButton::released,
            action, &QAction::trigger
        );
        toolbarLayout->addWidget(button);
    };

    addButton("Apply", this->actionApply);
    addButton("Reset", this->actionReset);
    addButton("Layout", this->actionLayout);

    {
        auto checkBox = new QCheckBox("Auto Layout");
        checkBox->setCheckState( Qt::Checked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                this->autoUpdateLayout = state;
                this->actionAutoLayoutZoom->trigger();
                return 1;
            }
        );
        toolbarLayout->addWidget(checkBox);
    }

    addButton("Zoom", actionZoom);

    {
        auto checkBox = new QCheckBox("Auto Zoom");
        checkBox->setCheckState( Qt::Checked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                this->autoUpdateZoom = state;
                this->actionAutoLayoutZoom->trigger();
                return 1;
            }
        );
        toolbarLayout->addWidget(checkBox);
    }

    // add spacer
    toolbarLayout->addItem( new QSpacerItem(0,0,QSizePolicy::Expanding) );

    return 1;
}

int NodeEditor::attachServerManagerListeners(){

    // retrieve server manager model (used for listening to proxy events)
    auto smm = pqApplicationCore::instance()->getServerManagerModel();

    // source/filter creation
    this->connect(
        smm, &pqServerManagerModel::sourceAdded,
        this, &NodeEditor::createNodeForSource
    );

    // source/filter deletion
    this->connect(
        smm, &pqServerManagerModel::sourceRemoved,
        this, &NodeEditor::removeNode
    );

    // view creation
    this->connect(
        smm, &pqServerManagerModel::viewAdded,
        this, &NodeEditor::createNodeForView
    );

    // view deletion
    this->connect(
        smm, &pqServerManagerModel::viewRemoved,
        this, &NodeEditor::removeNode
    );

    // edge creation
    this->connect(
        smm, &pqServerManagerModel::connectionAdded,
        this, [=](pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort){
            return this->updatePipelineEdges(consumer);
        }
    );

    // // retrieve active object manager
    auto activeObjects = &pqActiveObjects::instance();

    // update proxy selections
    this->connect(
        activeObjects, &pqActiveObjects::selectionChanged,
        this, &NodeEditor::updateActiveSourcesAndPorts
    );

    // update view selection
    this->connect(
        activeObjects, &pqActiveObjects::viewChanged,
        this, &NodeEditor::updateActiveView
    );

    for(auto proxy : smm->findItems<pqPipelineSource*>()){
        this->createNodeForSource(proxy);
    }

    for(auto proxy : smm->findItems<pqView*>()){
        this->createNodeForView(proxy);
        this->updateVisibilityEdges(proxy);
    }

    return 1;
}

int NodeEditor::updateActiveView(){
    for(auto it : this->nodeRegistry)
        if(dynamic_cast<pqView*>(it.second->getProxy()))
            it.second->setOutlineStyle(0);

    auto view = pqActiveObjects::instance().activeView();
    if(!view)
        return 1;

    this->nodeRegistry[ NE::getID(view) ]->setOutlineStyle(2);

    return 1;
}

int NodeEditor::updateActiveSourcesAndPorts(){
    NE::log("Selection Changed:");

    // unselect all nodes
    for(auto it : this->nodeRegistry){
        if(!dynamic_cast<pqPipelineSource*>(it.second->getProxy()))
            continue;

        it.second->setOutlineStyle(0);
        for(auto oPort : it.second->getOutputPorts())
            oPort->setStyle(0);
    }

    // select nodes in selection
    const auto& selection = pqActiveObjects::instance().selection();

    for(auto it : selection){
        if(auto source = dynamic_cast<pqPipelineSource*>(it)){
            NE::log("    -> source/filter");
            auto node = this->nodeRegistry[ NE::getID(source) ];
            node->setOutlineStyle(1);

            auto oPorts = node->getOutputPorts();
            if(oPorts.size()>0)
                oPorts[0]->setStyle(1);

        } else if(auto port = dynamic_cast<pqOutputPort*>(it)) {
            NE::log("    -> port");
            auto node = this->nodeRegistry[ NE::getID(port->getSource()) ];

            node->setOutlineStyle(1);
            node->getOutputPorts()[port->getPortNumber()]->setStyle(1);
        }
    }

    return 1;
}

NE::Node* NodeEditor::createNode(pqProxy* proxy){
    auto id = NE::getID(proxy);

    NE::log("Node Added: " + NE::getLabel(proxy));

    // insert new node into registry
    auto proxyAsView = dynamic_cast<pqView*>(proxy);
    auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

    auto node = proxyAsView
            ? new NE::Node(proxyAsView)
            : proxyAsSource
                ? new NE::Node(proxyAsSource)
                : nullptr;

    if(!node){
        NE::log("ERROR: Unable to create node for pqProxy.", true);
        return nullptr;
    }

    this->nodeRegistry.insert({ id, node });
    this->edgeRegistry.insert({ id, std::vector<NE::Edge*>() });

    this->scene->addItem(node);

    QObject::connect(
        node, &NE::Node::nodeResized,
        this->actionAutoLayoutZoom, &QAction::trigger
    );

    this->actionAutoLayoutZoom->trigger();

    return node;
}

int NodeEditor::createNodeForSource(pqPipelineSource* proxy){
    auto node = this->createNode(proxy);

    // update proxy selection
    QObject::connect(
        node, &NE::Node::nodeClicked,
        node, [=](QGraphicsSceneMouseEvent* event){
            auto proxy = node->getProxy();
            auto proxyAsSourceProxy = dynamic_cast<pqPipelineSource*>(proxy);
            if(!proxyAsSourceProxy)
                return 1;

            auto activeObjects = &pqActiveObjects::instance();

            // add to selection
            if(event->modifiers()==Qt::ControlModifier){
                pqProxySelection sel = activeObjects->selection();
                sel.push_back( proxy );
                activeObjects->setSelection(
                    sel,
                    proxy
                );
                return 1;
            }

            // make active selection
            activeObjects->setActiveSource( proxyAsSourceProxy );

            return 1;
        }
    );

    // update proxy selection
    QObject::connect(
        node, &NE::Node::portClicked,
        node, [=](QGraphicsSceneMouseEvent* event, int type, int idx){
            if(type!=1)
                return 1;

            auto proxy = node->getProxy();
            auto proxyAsSourceProxy = dynamic_cast<pqPipelineSource*>(proxy);
            if(!proxyAsSourceProxy)
                return 1;

            auto port = proxyAsSourceProxy->getOutputPort(idx);
            if(!port)
                return 1;

            auto activeObjects = &pqActiveObjects::instance();

            // add to selection
            if(event->modifiers()==Qt::ControlModifier){
                pqProxySelection sel = activeObjects->selection();
                sel.push_back( port );
                activeObjects->setSelection(
                    sel,
                    port
                );
                return 1;
            }

            // toggle visibility
            if(event->modifiers()==Qt::ShiftModifier){
                NE::log("change visibility of port "+std::to_string(idx));
                auto view = pqActiveObjects::instance().activeView();
                if(!view)
                    return 1;

                vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

                auto state = controller->GetVisibility(
                    port->getSourceProxy(),
                    port->getPortNumber(),
                    (vtkSMViewProxy*)view->getProxy()
                );

                controller->SetVisibility(
                    port->getSourceProxy(),
                    port->getPortNumber(),
                    (vtkSMViewProxy*)view->getProxy(),
                    !state
                );

                view->render();

                return 1;
            }

            // make active selection
            activeObjects->setActivePort( port );

            return 1;
        }
    );

    return 1;
};

int NodeEditor::createNodeForView(pqView* proxy){
    auto node = this->createNode(proxy);

    // update representation link
    QObject::connect(
        proxy, &pqView::representationVisibilityChanged,
        node, [=](pqRepresentation* rep, bool visible){
            return this->updateVisibilityEdges(proxy);
        }
    );

    // update proxy selection
    QObject::connect(
        node, &NE::Node::nodeClicked,
        [=](QGraphicsSceneMouseEvent* event){
            pqActiveObjects::instance().setActiveView( proxy );
            return 1;
        }
    );

    return 1;
};

int NodeEditor::removeNode(pqProxy* proxy){
    NE::log(
        "Proxy Removed: "+
        NE::getLabel(proxy)
    );

    // get id
    auto proxyId = NE::getID(proxy);

    // delete all incoming edges
    auto edges = this->edgeRegistry[ proxyId ];
    for(int i=0; i<edges.size(); i++)
        delete edges[i];
    edges.resize(0);
    this->edgeRegistry.erase( proxyId );

    // delete node
    delete this->nodeRegistry[ proxyId ];
    this->nodeRegistry.erase( proxyId );

    this->actionAutoLayoutZoom->trigger();

    return 1;
};

int NodeEditor::updateVisibilityEdges(pqView* proxy){
    NE::log(
        "Updating Visibility Pipeline Edges: "+
        NE::getLabel(proxy)
    );

    auto& viewEdges = this->edgeRegistry[ NE::getID(proxy) ];

    // delete all incoming edges
    for(int i=0; i<viewEdges.size(); i++)
        delete viewEdges[i];
    viewEdges.resize(0);

    auto nodes = this->nodeRegistry;

    for(int i=0; i<proxy->getNumberOfRepresentations(); i++){
        auto rep = proxy->getRepresentation(i);

        auto repAsDataRep = dynamic_cast<pqDataRepresentation*>(rep);
        if(!repAsDataRep || !repAsDataRep->isVisible())
            continue;

        auto producerPort = repAsDataRep->getOutputPortFromInput();
        auto producerNode = nodes[ NE::getID(producerPort->getSource()) ];

        auto viewNode = nodes[ NE::getID(proxy) ];

        // create edge
        auto edge = new NE::Edge(
            producerNode,
            producerPort->getPortNumber(),
            viewNode,
            0,
            1
        );
        viewEdges.push_back( edge );

        // add edge to scene
        this->scene->addItem(edge);
    }

    this->actionAutoLayoutZoom->trigger();

    return 1;
}

int NodeEditor::updatePipelineEdges(pqPipelineSource *consumer){
    NE::log(
        "Updating Incoming Pipeline Edges: "+
        NE::getLabel(consumer)
    );

    // check if consumer is actually a filter
    auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
    if(!consumerAsFilter){
        return 1;
    }

    // retrieve node of filter
    auto consumerNode = this->nodeRegistry[ NE::getID(consumerAsFilter) ];

    // retrieve all input edges
    auto& consumerInputEdges = this->edgeRegistry[ NE::getID(consumerAsFilter) ];

    // delete all incoming edges of consumer
    for(int i=0; i<consumerInputEdges.size(); i++)
        delete consumerInputEdges[i];
    consumerInputEdges.resize(0);

    // recreate all incoming edges
    for(int iPortIdx=0; iPortIdx<consumerAsFilter->getNumberOfInputPorts(); iPortIdx++){

        // retrieve current input port name
        auto iPortName = consumerAsFilter->getInputPortName(iPortIdx);

        // get number of all output ports connected to current input port
        int numberOfOutputPortsAtInputPort = consumerAsFilter->getNumberOfInputs( iPortName );
        for(int oPortIt=0; oPortIt<numberOfOutputPortsAtInputPort; oPortIt++){
            // get current output port connected to current input port
            auto producerPort = consumerAsFilter->getInput( iPortName, oPortIt );

            // get source of current output port
            auto producer = producerPort->getSource();

            // get node of source
            auto producerNode = this->nodeRegistry[ NE::getID(producer) ];

            // create edge
            auto edge = new NE::Edge(
                producerNode,
                producerPort->getPortNumber(),
                consumerNode,
                iPortIdx
            );
            consumerInputEdges.push_back( edge );
            // add edge to scene
            this->scene->addItem(edge);
        }
    }

    this->actionAutoLayoutZoom->trigger();

    return 1;
};

