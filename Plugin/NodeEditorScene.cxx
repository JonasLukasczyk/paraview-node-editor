#include <NodeEditorScene.h>

// node editor includes
#include <Node.h>
#include <Edge.h>
#include <Port.h>

// qt includes
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

// paraview/vtk includes
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqRepresentation.h>
#include <pqDataRepresentation.h>
#include <pqActiveObjects.h>
#include <pqOutputPort.h>
#include <pqView.h>
#include <vtkSMProxy.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>

// std includes
#include <iostream>
#include <sstream>

NE::NodeEditorScene::NodeEditorScene(QObject* parent) : QGraphicsScene(parent){

    // retrieve server manager model (used for listening to proxy events)
    auto core = pqApplicationCore::instance();
    auto smm = core->getServerManagerModel();

    // source/filter creation
    this->connect(
        smm, &pqServerManagerModel::sourceAdded,
        this, &NodeEditorScene::createNodeForSource
    );

    // source/filter deletion
    this->connect(
        smm, &pqServerManagerModel::sourceRemoved,
        this, &NodeEditorScene::removeNode
    );

    // view creation
    this->connect(
        smm, &pqServerManagerModel::viewAdded,
        this, &NodeEditorScene::createNodeForView
    );

    // view deletion
    this->connect(
        smm, &pqServerManagerModel::viewRemoved,
        this, &NodeEditorScene::removeNode
    );

    // edge creation
    this->connect(
        smm, &pqServerManagerModel::connectionAdded,
        this, &NodeEditorScene::createEdges
    );

    // retrieve active object manager
    auto activeObjects = &pqActiveObjects::instance();

    // update proxy selections
    this->connect(
        activeObjects, &pqActiveObjects::selectionChanged,
        this,
        [=](const pqProxySelection &selection){
            std::cout<<"selection changed"<<std::endl;

            // unselect all nodes
            for(auto it : this->nodeRegistry){
                if(!dynamic_cast<pqPipelineSource*>(it.second->getProxy()))
                    continue;

                it.second->setOutlineStyle(0);
                for(auto oPort : it.second->getOutputPorts())
                    oPort->setStyle(0);
            }

            // select nodes in selection
            for(auto it : selection){
                if(auto source = dynamic_cast<pqPipelineSource*>(it)){
                    std::cout<<"    -> source"<<std::endl;
                    auto node = this->nodeRegistry[ source->getProxy()->GetGlobalID() ];
                    node->setOutlineStyle(1);

                    auto oPorts = node->getOutputPorts();
                    if(oPorts.size()>0)
                        oPorts[0]->setStyle(1);

                } else if(auto port = dynamic_cast<pqOutputPort*>(it)) {
                    std::cout<<"    -> port"<<std::endl;
                    auto node = this->nodeRegistry[ port->getSource()->getProxy()->GetGlobalID() ];

                    node->setOutlineStyle(1);
                    node->getOutputPorts()[port->getPortNumber()]->setStyle(1);
                }
            }

            return 1;
        }
    );

    // update view selection
    this->connect(
        activeObjects, &pqActiveObjects::viewChanged,
        this,
        [=](pqView* view){

            for(auto it : this->nodeRegistry)
               if(dynamic_cast<pqView*>(it.second->getProxy()))
                    it.second->setOutlineStyle(0);

            if(!view)
                return 1;

            this->nodeRegistry[ view->getProxy()->GetGlobalID() ]->setOutlineStyle(2);

            return 1;
        }
    );
}

NE::NodeEditorScene::~NodeEditorScene(){
}

#if NE_ENABLE_GRAPHVIZ
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif

int NE::NodeEditorScene::computeLayout(){

    std::cout<<"Computing Graph Layout"<<std::endl;

#if NE_ENABLE_GRAPHVIZ

    std::stringstream headString;
    std::stringstream nodeString;
    std::stringstream edgeString;
    std::stringstream rank0String;
    std::stringstream rank1String;

    // head
    headString<<"digraph g {rankdir=TB;\n";
    rank0String<<"{rank=same ";
    rank1String<<"{rank=same ";

    // nodes
    for(auto it : this->nodeRegistry){
        const auto& b = it.second->boundingRect();
        nodeString
            << it.second->getProxy()->getProxy()->GetGlobalID()
            << "["
            << "label=\"\","
            << "shape=box,"
            << "width="<<(b.width()/100+2)<<","
            << "height="<<(b.height()/100+4)<<""
            <<"];\n";

        for(auto edge : this->edgeRegistry[it.second->getProxy()->getProxy()->GetGlobalID()]){
            edgeString
                << edge->getProducer()->getProxy()->getProxy()->GetGlobalID()
                << " -> "
                << edge->getConsumer()->getProxy()->getProxy()->GetGlobalID()
                << ";\n";
        }

        auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
        if(proxyAsView)
            rank1String<<it.second->getProxy()->getProxy()->GetGlobalID()<< " ";
        else
            rank0String<<it.second->getProxy()->getProxy()->GetGlobalID()<< " ";
    }

    auto dotString = (headString.str()+nodeString.str()+edgeString.str()+rank0String.str()+"}\n"+rank1String.str()+"}\n"+"}");
    // std::cout<<dotString<<std::endl;

    // init layout
    Agraph_t *G = agmemread(
        dotString.data()
    );
    GVC_t *gvc = gvContext();
    gvLayout(gvc, G, "dot");

    // read layout
    for(auto it : this->nodeRegistry){
        Agnode_t *n = agnode(G, const_cast<char *>(std::to_string( it.second->getProxy()->getProxy()->GetGlobalID() ).data()), 0);
        if(n != nullptr) {
            auto &coord = ND_coord(n);
            it.second->setPos(coord.x, -coord.y);
        }
    }

    // delete layout
    gvFreeLayout(gvc, G);
    agclose(G);
    gvFreeContext(gvc);

    return 1;
#else
    std::cout<<"ERROR: GraphViz support disabled!"<<std::endl;
    return 0;
#endif
}

QRect NE::NodeEditorScene::getBoundingRect(){
    int x0 = 999999;
    int x1 = -999999;
    int y0 = 999999;
    int y1 = -999999;

    for(auto it : this->nodeRegistry){
        auto p = it.second->pos();
        auto b = it.second->boundingRect();
        if(x0>p.x()+b.left())
            x0=p.x()+b.left();
        if(x1<p.x()+b.right())
            x1=p.x()+b.right();
        if(y0>p.y()+b.top())
            y0=p.y()+b.top();
        if(y1<p.y()+b.bottom())
            y1=p.y()+b.bottom();
    }

    return QRect(x0,y0,x1-x0,y1-y0);
}

NE::Node* NE::NodeEditorScene::createNode(pqProxy* proxy){
    std::cout
        <<"Node Added: "
        <<proxy->getSMName().toStdString()
        <<"<"<<proxy->getProxy()->GetGlobalID()<<">"
        <<std::endl;

    // insert new node into registry
    auto proxyAsView = dynamic_cast<pqView*>(proxy);
    auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy);
    auto nodeIt = this->nodeRegistry.insert({
        proxy->getProxy()->GetGlobalID(),
        proxyAsView
            ? new NE::Node(proxyAsView)
            : proxyAsSource
                ? new NE::Node(proxyAsSource)
                : new NE::Node(proxy)
    });
    auto node = nodeIt.first->second;

    // prepare input edges for new node
    auto edges = this->edgeRegistry.insert({
        proxy->getProxy()->GetGlobalID(),
        std::vector<Edge*>()
    });

    // add node to scene
    this->addItem(node);

    // update proxy selection
    QObject::connect(
        node, &NE::Node::nodeResized,
        [=](){ emit nodesModified(); }
    );

    emit nodesModified();

    return node;
}

int NE::NodeEditorScene::createNodeForSource(pqPipelineSource* proxy){
    auto node = this->createNode(proxy);

    // update proxy selection
    QObject::connect(
        node, &NE::Node::nodeClicked,
        node, [=](QGraphicsSceneMouseEvent* event){
            auto proxy = node->getProxy();
            auto proxyAsSourceProxy = dynamic_cast<pqPipelineSource*>(proxy);
            if(!proxyAsSourceProxy)
                return 1;

            // change visibility
            if(event->modifiers()==Qt::ShiftModifier){
                std::cout<<"change visibility"<<std::endl;
                auto view = pqActiveObjects::instance().activeView();
                if(!view)
                    return 1;

                vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

                for(int portIdx=0; portIdx<proxyAsSourceProxy->getNumberOfOutputPorts(); portIdx++){
                    auto port = proxyAsSourceProxy->getOutputPort(portIdx);

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
                }

                return 1;
            }

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

            // make active selection
            activeObjects->setActivePort( port );

            return 1;
        }
    );

    return 1;
};

int NE::NodeEditorScene::createNodeForView(pqView* proxy){

    auto node = this->createNode(proxy);

    // update representation link
    QObject::connect(
        proxy, &pqView::representationVisibilityChanged,
        node, [=](pqRepresentation* rep, bool visible){

            auto& viewEdges = this->edgeRegistry[ proxy->getProxy()->GetGlobalID() ];

            // delete all incoming edges
            for(int i=0; i<viewEdges.size(); i++)
                delete viewEdges[i];
            viewEdges.resize(0);

            for(int i=0; i<proxy->getNumberOfRepresentations(); i++){
                auto rep = proxy->getRepresentation(i);

                auto repAsDataRep = dynamic_cast<pqDataRepresentation*>(rep);
                if(!repAsDataRep || !repAsDataRep->isVisible())
                    continue;

                auto producerPort = repAsDataRep->getOutputPortFromInput();
                auto producerNode = this->nodeRegistry[ producerPort->getSource()->getProxy()->GetGlobalID() ];

                auto viewNode = this->nodeRegistry[ proxy->getProxy()->GetGlobalID() ];

                // create edge
                auto edge = new Edge(
                    producerNode,
                    producerPort->getPortNumber(),
                    viewNode,
                    0,
                    1
                );
                viewEdges.push_back( edge );

                // add edge to scene
                this->addItem(edge);
            }

            return 1;
        }
    );

    // update proxy selection
    QObject::connect(
        node, &NE::Node::nodeClicked,
        [=](QGraphicsSceneMouseEvent* event){
            auto proxy = node->getProxy();
            auto proxyAsView = dynamic_cast<pqView*>(proxy);

            pqActiveObjects* activeObjects = &pqActiveObjects::instance();

            if(proxyAsView)
                activeObjects->setActiveView( proxyAsView );
        }
    );

    return 1;
};

int NE::NodeEditorScene::removeNode(pqProxy* proxy){
    std::cout
        <<"Proxy Removed: "
        <<proxy->getSMName().toStdString()
        <<"<"<<proxy->getProxy()->GetGlobalID()<<">"
        <<std::endl;

    // get id
    auto proxyId = proxy->getProxy()->GetGlobalID();

    // delete all incoming edges
    auto edges = this->edgeRegistry[ proxyId ];
    for(int i=0; i<edges.size(); i++)
        delete edges[i];
    edges.resize(0);
    this->edgeRegistry.erase( proxyId );

    // delete node
    delete this->nodeRegistry[ proxyId ];
    this->nodeRegistry.erase( proxyId );

    emit nodesModified();

    return 1;
};

int NE::NodeEditorScene::createEdges(pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort){
    std::cout<<"Connection Added: "
        << source->getSMName().toStdString()
        <<"<"<<source->getProxy()->GetGlobalID()<<">"
        <<" -> "
        << consumer->getSMName().toStdString()
        <<"<"<<consumer->getProxy()->GetGlobalID()<<">"
        <<std::endl;

    // check if source is actually a filter
    auto consumerAsFilter = dynamic_cast<pqPipelineFilter*>(consumer);
    if(!consumerAsFilter){
        std::cout<<"[ERROR] Unable to cast consumer to pqPipelineFilter."<<std::endl;
        return 0;
    }

    // retrieve node of filter
    auto consumerNode = this->nodeRegistry[ consumerAsFilter->getProxy()->GetGlobalID() ];
    if(consumerNode==nullptr){
        std::cout<<"[ERROR] WTF"<<std::endl;
        return 0;
    }

    // retrieve all input edges
    auto& consumerInputEdges = this->edgeRegistry[ consumerAsFilter->getProxy()->GetGlobalID() ];

    // delete all incoming edges of consumer
    for(int i=0; i<consumerInputEdges.size(); i++)
        delete consumerInputEdges[i];
    consumerInputEdges.resize(0);

    // recreate all incoming edges
    for(int iPortIdx=0; iPortIdx<consumerAsFilter->getNumberOfInputPorts(); iPortIdx++){

        // retrieve current input port name
        auto iPortName = consumerAsFilter->getInputPortName(iPortIdx);
        if(iPortName.isEmpty()){
            std::cout<<"[ERROR] input port has no name."<<std::endl;
            return 0;
        }

        // get number of all output ports connected to current input port
        int numberOfOutputPortsAtInputPort = consumerAsFilter->getNumberOfInputs( iPortName );
        for(int oPortIt=0; oPortIt<numberOfOutputPortsAtInputPort; oPortIt++){
            // get current output port connected to current input port
            auto producerPort = consumerAsFilter->getInput( iPortName, oPortIt );

            // get source of current output port
            auto producer = producerPort->getSource();

            // get node of source
            auto producerNode = this->nodeRegistry[producer->getProxy()->GetGlobalID()];
            if(!producerNode){
                std::cout<<"[ERROR] Unable to retrieve producer node for connection."<<std::endl;
                return 0;
            }

            // create edge
            auto edge = new Edge(
                producerNode,
                producerPort->getPortNumber(),
                consumerNode,
                iPortIdx
            );
            consumerInputEdges.push_back( edge );
            // add edge to scene
            this->addItem(edge);
        }
    }

    emit edgesModified();

    return 1;
};

void NE::NodeEditorScene::drawBackground(QPainter *painter, const QRectF &rect){
    const int gridSize = 25;

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    QVarLengthArray<QLineF, 100> lines;

    for (qreal x = left; x < rect.right(); x += gridSize)
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y < rect.bottom(); y += gridSize)
        lines.append(QLineF(rect.left(), y, rect.right(), y));

    painter->setRenderHints(QPainter::HighQualityAntialiasing);
    painter->setPen( QColor(60,60,60) );
    painter->drawLines(lines.data(), lines.size());
}