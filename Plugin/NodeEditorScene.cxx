#include <NodeEditorScene.h>

// node editor includes
#include <Node.h>
#include <Edge.h>

// qt includes
#include <QPainter>

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

// std includes
#include <iostream>

NodeEditorScene::NodeEditorScene(QObject* parent) : QGraphicsScene(parent){

    // retrieve server manager model (used for listening to proxy events)
    auto core = pqApplicationCore::instance();
    auto smm = core->getServerManagerModel();

    // source/filter creation
    this->connect(
        smm, &pqServerManagerModel::sourceAdded,
        this, &NodeEditorScene::createNode
    );

    // source/filter deletion
    this->connect(
        smm, &pqServerManagerModel::sourceRemoved,
        this, &NodeEditorScene::removeNode
    );

    // representation created
    this->connect(
        smm, &pqServerManagerModel::representationAdded,
        this,
        [=](pqRepresentation* rep){

            auto repAsDataRep = dynamic_cast<pqDataRepresentation*>(rep);
            if(!repAsDataRep)
                return 1;

            auto proxy = rep->getProxy();
            std::cout
            <<"Representation Added: "
            <<rep->getSMName().toStdString()
            <<"<"<<proxy->GetGlobalID()<<">"
            <<"["<<proxy->GetNumberOfProducers()<<"]"
            <<"["<<proxy->GetNumberOfConsumers()<<"]"
            <<std::endl;

            for(int i=0; i<proxy->GetNumberOfProducers(); i++){
                auto producerProxy = proxy->GetProducerProxy(i);
                std::cout
                    <<"    "
                    <<i<<": "
                    <<producerProxy->GetXMLName()
                    <<"("<<producerProxy->GetVTKClassName()<<")"
                    <<"<"<<producerProxy->GetGlobalID()<<">"
                    <<std::endl;
            }

            return 1;
        }
    );

    // edge creation
    this->connect(
        smm, &pqServerManagerModel::connectionAdded,
        this, &NodeEditorScene::createEdges
    );

    // update proxy selections
    pqActiveObjects* activeObjects = &pqActiveObjects::instance();
    this->connect(
        activeObjects, &pqActiveObjects::selectionChanged,
        [=](const pqProxySelection &selection){
            for(auto it : this->nodeRegistry)
                it.second->setState(0);

            for(auto it : selection){
                auto source = dynamic_cast<pqPipelineSource*>(it);
                if(source){
                    this->nodeRegistry[ source->getProxy()->GetGlobalID() ]->setState(1);
                }
            }
        }
    );
}

NodeEditorScene::~NodeEditorScene(){
}

int NodeEditorScene::createNode(pqPipelineSource* source){
    std::cout
        <<"Source Added: "
        <<source->getSMName().toStdString()
        <<"<"<<source->getProxy()->GetGlobalID()<<">"
        <<std::endl;

    // insert new node into registry
    auto nodeIt = this->nodeRegistry.insert({
        source->getProxy()->GetGlobalID(),
        new Node(source)
    });

    // prepare input edges for new node
    auto edges = this->edgeRegistry.insert({
        source->getProxy()->GetGlobalID(),
        std::vector<Edge*>()
    });

    // add node to scene
    this->addItem(nodeIt.first->second);

    // update proxy selection
    QObject::connect(
        nodeIt.first->second, &Node::nodeClicked,
        [=](){
            auto proxy = nodeIt.first->second->getProxy();
            auto proxyAsSourceProxy = dynamic_cast<pqPipelineSource*>(proxy);

            pqActiveObjects* activeObjects = &pqActiveObjects::instance();

            if(proxyAsSourceProxy)
                activeObjects->setActiveSource( proxyAsSourceProxy );
        }
    );

    return 1;
};

int NodeEditorScene::removeNode(pqPipelineSource* source){
    std::cout
        <<"Source Removed: "
        <<source->getSMName().toStdString()
        <<"<"<<source->getProxy()->GetGlobalID()<<">"
        <<std::endl;

    // get id
    auto sourceId = source->getProxy()->GetGlobalID();

    // delete all incoming edges
    auto edges = this->edgeRegistry[ sourceId ];
    for(int i=0; i<edges.size(); i++)
        delete edges[i];
    edges.resize(0);
    this->edgeRegistry.erase( sourceId );

    // delete node
    delete this->nodeRegistry[ sourceId ];
    this->nodeRegistry.erase( sourceId );

    return 1;
};

int NodeEditorScene::createEdges(pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort){
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

    return 1;
};

void NodeEditorScene::drawBackground(QPainter *painter, const QRectF &rect){
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