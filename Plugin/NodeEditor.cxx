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
#include <pqUndoStack.h>

#include <vtkSMInputProperty.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMTrace.h>

// for state files
#include <vtkPVXMLElement.h>
#include <vtkSMProxyLocator.h>

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

    this->actionAutoLayout = new QAction(this);
    QObject::connect(
        this->actionAutoLayout, &QAction::triggered,
        this->scene, [=](){
            if(this->autoUpdateLayout)
                this->actionLayout->trigger();
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
        return 1;
    };

    addButton("Apply", this->actionApply);
    addButton("Reset", this->actionReset);

    addButton("Layout", this->actionLayout);
    {
        auto checkBox = new QCheckBox("Auto Layout");
        checkBox->setCheckState( this->autoUpdateLayout ? Qt::Checked : Qt::Unchecked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                this->autoUpdateLayout = state;
                this->actionAutoLayout->trigger();
                return 1;
            }
        );
        toolbarLayout->addWidget(checkBox);
    }

    addButton("Zoom", actionZoom);

    {
        auto checkBox = new QCheckBox("Debug");
        checkBox->setCheckState( NE::CONSTS::DEBUG ? Qt::Checked : Qt::Unchecked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                NE::CONSTS::DEBUG = state;
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
    auto appCore = pqApplicationCore::instance();
    auto smm = appCore->getServerManagerModel();

    // state loaded
    this->connect(
        appCore, &pqApplicationCore::aboutToLoadState,
        this, [=](vtkPVXMLElement* root){
            // PV BUG: does not fire
        }
    );
    this->connect(
        appCore, &pqApplicationCore::stateLoaded,
        this, [=](vtkPVXMLElement* root, vtkSMProxyLocator* locator){
            this->actionLayout->trigger();
            this->actionZoom->trigger();
        }
    );

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

    // edge removed
    this->connect(
        smm, &pqServerManagerModel::connectionRemoved,
        this, [=](pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort){
            return this->updatePipelineEdges(consumer);
        }
    );

    // edge creation
    this->connect(
        smm, &pqServerManagerModel::connectionAdded,
        this, [=](pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort){
            return this->updatePipelineEdges(consumer);
        }
    );

    // retrieve active object manager
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

    // init node editor scene with exisiting views
    {
        for(auto proxy : smm->findItems<pqPipelineSource*>()){
            this->createNodeForSource(proxy);
            this->updatePipelineEdges(proxy);
        }

        for(auto proxy : smm->findItems<pqView*>()){
            this->createNodeForView(proxy);
            this->updateVisibilityEdges(proxy);
            this->updateActiveView();
        }
    }

    return 1;
}

int NodeEditor::updateActiveView(){
    NE::log("Update Active View");

    for(auto it : this->nodeRegistry)
        if(dynamic_cast<pqView*>(it.second->getProxy()))
            it.second->setOutlineStyle(0);

    auto view = pqActiveObjects::instance().activeView();
    if(!view)
        return 1;

    auto nodeIt = this->nodeRegistry.find( NE::getID(view) );
    if(nodeIt==this->nodeRegistry.end())
        return 1;

    nodeIt->second->setOutlineStyle(2);

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
    const auto selection = pqActiveObjects::instance().selection();

    for(auto it : selection){
        if(auto source = dynamic_cast<pqPipelineSource*>(it)){
            NE::log("    -> source/filter");

            auto nodeIt = this->nodeRegistry.find( NE::getID(source) );
            if(nodeIt==this->nodeRegistry.end())
                continue;

            nodeIt->second->setOutlineStyle(1);

            auto oPorts = nodeIt->second->getOutputPorts();
            if(oPorts.size()>0)
                oPorts[0]->setStyle(1);

        } else if(auto port = dynamic_cast<pqOutputPort*>(it)) {
            NE::log("    -> port");
            auto nodeIt = this->nodeRegistry.find( NE::getID(port->getSource()) );
            if(nodeIt==this->nodeRegistry.end())
                continue;

            nodeIt->second->setOutlineStyle(1);
            nodeIt->second->getOutputPorts()[port->getPortNumber()]->setStyle(1);
        }
    }

    return 1;
}

NE::Node* NodeEditor::createNode(pqProxy* proxy){
    NE::log("Proxy Added: "+NE::getLabel(proxy));

    auto id = NE::getID(proxy);

    // insert new node into registry
    auto proxyAsView = dynamic_cast<pqView*>(proxy);
    auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);

    auto node = proxyAsView
            ? new NE::Node(this->scene, proxyAsView)
            : proxyAsSource
                ? new NE::Node(this->scene, proxyAsSource)
                : nullptr;

    if(!node){
        NE::log("ERROR: Unable to create node for pqProxy.", true);
        return nullptr;
    }

    this->nodeRegistry.insert({ id, node });
    this->edgeRegistry.insert({ id, std::vector<NE::Edge*>() });

    QObject::connect(
        node, &NE::Node::nodeResized,
        this->actionAutoLayout, &QAction::trigger
    );

    auto activeProxy = pqActiveObjects::instance().activeSource();
    if(activeProxy){
       auto prevNodeIt = this->nodeRegistry.find(
           NE::getID(activeProxy)
       );
       if(prevNodeIt!=this->nodeRegistry.end()){
           auto prevPos = prevNodeIt->second->pos();
           node->setPos(
               prevPos.x()+350,
               prevPos.y()
           );
       }
    }

    this->actionAutoLayout->trigger();

    return node;
}

int NodeEditor::createNodeForSource(pqPipelineSource* proxy){
    auto node = this->createNode(proxy);

    // update proxy selection
    {
        node->getLabel()->installEventFilter(
            NE::createInterceptor(
                node->getLabel(),
                [=](QObject* object, QEvent* event){
                    if(event->type()!=QEvent::GraphicsSceneMousePress)
                        return false;
                    auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

                    // double left click
                    if(eventMDC->button()==1 && NE::isDoubleClick()){
                        auto activeObjects = &pqActiveObjects::instance();

                        // add to selection or make single active selection
                        if(eventMDC->modifiers()==Qt::ControlModifier){
                            pqProxySelection sel = activeObjects->selection();
                            sel.push_back( proxy );
                            activeObjects->setSelection(
                                sel,
                                proxy
                            );
                        } else {
                            activeObjects->setActiveSource( proxy );
                        }

                        return false;
                    }

                    // single right click
                    if(eventMDC->button()==2){
                        node->advanceVerbosity();
                        return false;
                    }

                    return false;
                }
            )
        );
    }

    // input port events
    if(auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy)){
        for(size_t idx=0; idx<node->getInputPorts().size(); idx++){
            auto nodePort = node->getInputPorts()[idx];
            nodePort->getLabel()->installEventFilter(
                NE::createInterceptor(
                    nodePort->getLabel(),
                    [=](QObject* object, QEvent* event){
                        if(event->type()!=QEvent::GraphicsSceneMousePress)
                            return false;

                        if(NE::isDoubleClick())
                            this->setActivePortAsInput(proxy, idx);

                        return false;
                    }
                )
            );
        }
    }

    // output port events
    {
        for(size_t idx=0; idx<node->getOutputPorts().size(); idx++){
            auto nodePort = node->getOutputPorts()[idx];
            nodePort->getLabel()->installEventFilter(
                NE::createInterceptor(
                    nodePort->getLabel(),
                    [=](QObject* object, QEvent* event){
                        if(event->type()!=QEvent::GraphicsSceneMousePress)
                            return false;

                        auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

                        auto portProxy = proxy->getOutputPort(idx);

                        // double left click
                        if(eventMDC->button()==1 && NE::isDoubleClick()){
                            auto activeObjects = &pqActiveObjects::instance();

                            // add to selection or make single active selection
                            if(eventMDC->modifiers()==Qt::ControlModifier){
                                pqProxySelection sel = activeObjects->selection();
                                sel.push_back( portProxy );
                                activeObjects->setSelection(
                                    sel,
                                    portProxy
                                );
                            } else {
                                activeObjects->setActivePort( portProxy );
                            }

                            return true;
                        }

                        // toggle visibility
                        if(eventMDC->button()==1 && eventMDC->modifiers()&Qt::ShiftModifier){
                            NE::log("Change Visibility of Port:");
                            NE::log("    "+std::to_string(idx));

                            // exclusive
                            if(eventMDC->modifiers()&Qt::ControlModifier)
                                this->hideAllInActiveView();

                            this->toggleInActiveView(
                                proxy->getOutputPort(idx)
                            );

                            return true;
                        }

                        return false;
                    }
                )
            );
        }
    }

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

    //update proxy selection
    node->getLabel()->installEventFilter(
        NE::createInterceptor(
            node->getLabel(),
            [=](QObject* object, QEvent* event){
                if(event->type()!=QEvent::GraphicsSceneMousePress)
                    return false;

                auto eventMDC = static_cast<QGraphicsSceneMouseEvent*>(event);

                // double left click
                if(eventMDC->button()==1 && NE::isDoubleClick())
                    pqActiveObjects::instance().setActiveView( proxy );

                return false;
            }
        )
    );

    return 1;
};

int NodeEditor::removeIncomingEdges(pqProxy* proxy){
    auto edgesIt = this->edgeRegistry.find( NE::getID(proxy) );
    if(edgesIt!=this->edgeRegistry.end()){
        for(int i=0; i<edgesIt->second.size(); i++)
            if(edgesIt->second[i])
                delete edgesIt->second[i];
        edgesIt->second.resize(0);
    }
    return 1;
}
int NodeEditor::removeNode(pqProxy* proxy){
    NE::log("Proxy Removed: "+NE::getLabel(proxy));

    // remove all visibility edges
    auto smm = pqApplicationCore::instance()->getServerManagerModel();
    for(auto view : smm->findItems<pqView*>())
        this->removeIncomingEdges(view);

    // get id
    auto proxyId = NE::getID(proxy);

    // delete all incoming edges
    removeIncomingEdges(proxy);
    this->edgeRegistry.erase( proxyId );

    // delete node
    auto nodeIt = this->nodeRegistry.find( proxyId );
    if(nodeIt!=this->nodeRegistry.end())
        delete nodeIt->second;
    this->nodeRegistry.erase( proxyId );

    // update visibility edges
    for(auto view : smm->findItems<pqView*>())
        this->updateVisibilityEdges(view);

    this->actionAutoLayout->trigger();

    return 1;
};

int NodeEditor::setActivePortAsInput(pqPipelineSource *consumer, int idx){
    NE::log("Set Active Port as Input: "+NE::getLabel(consumer));

    auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
    if(!consumerAsFilter)
        return 1;

    auto activeObjects = &pqActiveObjects::instance();
    auto port = activeObjects->activePort();
    if(!port)
        return 1;

    BEGIN_UNDO_SET(QString("Change Input for %1").arg(consumerAsFilter->getSMName()));
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", consumerAsFilter->getProxy());

    std::vector<vtkSMProxy*> inputPtrs;
    std::vector<unsigned int> inputPorts;

    inputPtrs.push_back(port->getSource()->getProxy());
    inputPorts.push_back(port->getPortNumber());

    auto iPortName = consumerAsFilter->getInputPortName(idx);
    auto ip = vtkSMInputProperty::SafeDownCast(
      consumerAsFilter->getProxy()->GetProperty(iPortName.toLocal8Bit().data())
    );
    ip->SetProxies(
        static_cast<unsigned int>(inputPtrs.size()),
        &inputPtrs[0],
        &inputPorts[0]
    );

    END_UNDO_SET();

    consumer->setModifiedState( pqProxy::ModifiedState::MODIFIED );

    this->actionAutoLayout->trigger();

    return 1;
};

int NodeEditor::toggleInActiveView(pqOutputPort* port){
    auto view = pqActiveObjects::instance().activeView();
    if(!view)
        return 0;

    auto viewSMProxy = static_cast<vtkSMViewProxy*>(view->getProxy());
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

    auto state = controller->GetVisibility(
        port->getSourceProxy(),
        port->getPortNumber(),
        viewSMProxy
    );

    controller->SetVisibility(
        port->getSourceProxy(),
        port->getPortNumber(),
        viewSMProxy,
        !state
    );

    view->render();

    return 1;
};

int NodeEditor::hideAllInActiveView(){
    auto view = pqActiveObjects::instance().activeView();
    if(!view)
        return 0;

    auto viewSMProxy = static_cast<vtkSMViewProxy*>(view->getProxy());
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

    for(auto nodeIt : this->nodeRegistry){
        auto proxy = static_cast<vtkSMSourceProxy*>(
            nodeIt.second->getProxy()->getProxy()
        );
        for(size_t jdx=0; jdx<proxy->GetNumberOfOutputPorts(); jdx++)
            controller->SetVisibility(
                proxy,
                jdx,
                viewSMProxy,
                false
            );
    }

    view->render();

    return 1;
};

int NodeEditor::updateVisibilityEdges(pqView* proxy){
    NE::log("Updating Visibility Pipeline Edges: "+NE::getLabel(proxy));

    this->removeIncomingEdges(proxy);

    auto viewEdgesIt = this->edgeRegistry.find( NE::getID(proxy) );
    if(viewEdgesIt==this->edgeRegistry.end())
        return 1;

    for(int i=0; i<proxy->getNumberOfRepresentations(); i++){
        auto rep = proxy->getRepresentation(i);
        if(!rep)
            continue;

        auto repAsDataRep = dynamic_cast<pqDataRepresentation*>(rep);
        if(!repAsDataRep || !repAsDataRep->isVisible())
            continue;

        auto producerPort = repAsDataRep->getOutputPortFromInput();
        auto producerIt = this->nodeRegistry.find( NE::getID(producerPort->getSource()) );
        if(producerIt==this->nodeRegistry.end())
            continue;

        auto viewIt = this->nodeRegistry.find( NE::getID(proxy) );
        if(viewIt==this->nodeRegistry.end())
            continue;

        // create edge
        viewEdgesIt->second.push_back(
            new NE::Edge(
                this->scene,
                producerIt->second,
                producerPort->getPortNumber(),
                viewIt->second,
                0,
                1
            )
        );
    }

    this->actionAutoLayout->trigger();

    return 1;
}

int NodeEditor::updatePipelineEdges(pqPipelineSource *consumer){
    NE::log("Updating Incoming Pipeline Edges: "+NE::getLabel(consumer));

    // check if consumer is actually a filter
    auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
    if(!consumerAsFilter){
        return 1;
    }

    // get node of consumer
    auto consumerIt = this->nodeRegistry.find( NE::getID(consumer) );
    if(consumerIt==this->nodeRegistry.end()){
        return 1;
    }

    // remove all input edges
    this->removeIncomingEdges(consumer);

    auto consumerEdgesIt = this->edgeRegistry.find( NE::getID(consumer) );
    if(consumerEdgesIt==this->edgeRegistry.end()){
        return 1;
    }

    // recreate all incoming edges
    for(int iPortIdx=0; iPortIdx<consumerAsFilter->getNumberOfInputPorts(); iPortIdx++){

        // retrieve current input port name
        auto iPortName = consumerAsFilter->getInputPortName(iPortIdx);

        // get number of all outconsumerNodeput ports connected to current input port
        int numberOfOutputPortsAtInputPort = consumerAsFilter->getNumberOfInputs( iPortName );
        for(int oPortIt=0; oPortIt<numberOfOutputPortsAtInputPort; oPortIt++){
            // get current output port connected to current input port
            auto producerPort = consumerAsFilter->getInput( iPortName, oPortIt );

            // get source of current output port
            auto producer = producerPort->getSource();

            // get node of producer
            auto producerIt = this->nodeRegistry.find( NE::getID(producer) );
            if(producerIt==this->nodeRegistry.end()){
                continue;
            }

            // create edge
            consumerEdgesIt->second.push_back(
                new NE::Edge(
                    this->scene,
                    producerIt->second,
                    producerPort->getPortNumber(),
                    consumerIt->second,
                    iPortIdx
                )
            );
        }
    }

    this->actionAutoLayout->trigger();

    return 1;
};

