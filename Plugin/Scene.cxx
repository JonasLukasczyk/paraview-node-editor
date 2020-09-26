#include <Scene.h>

// node editor includes
#include <Node.h>
#include <Edge.h>
#include <Utils.h>

// qt includes
#include <QPainter>

// paraview/vtk includes
#include <pqPipelineSource.h>
#include <pqView.h>

// std includes
#include <sstream>

NE::Scene::Scene(QObject* parent) : QGraphicsScene(parent){
}

NE::Scene::~Scene(){
}

#if NE_ENABLE_GRAPHVIZ
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif

int NE::Scene::computeLayout(
    std::unordered_map<int,NE::Node*>& nodes,
    std::unordered_map<int,std::vector<NE::Edge*>>& edges
){
    NE::log("Computing Graph Layout");

#if NE_ENABLE_GRAPHVIZ

    // compute dot string
    qreal maxHeight = 3.0;
    qreal maxY = 0;
    std::string dotString;
    {
        std::stringstream nodeString;
        std::stringstream edgeString;

        for(auto it : nodes){
            auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
            if(proxyAsView)
                continue;

            const auto& b = it.second->boundingRect();
            qreal width = b.width()/100.0;
            qreal height = b.height()/100.0;
            if(maxHeight<height)
                maxHeight=height;

            nodeString
                << NE::getID(it.second->getProxy())
                << "["
                << "label=\"\","
                << "shape=box,"
                << "width="<<width<<","
                << "height="<<height<<""
                <<"];\n";

            for(auto edge : edges[ NE::getID(it.second->getProxy()) ]){
                edgeString
                    << NE::getID(edge->getProducer()->getProxy())
                    << " -> "
                    << NE::getID(edge->getConsumer()->getProxy())
                    << ";\n";
            }
        }

        dotString += std::string("")
            + "digraph g {\n"
            + "rankdir=LR;graph[pad=\"0.5\", ranksep=\"2\", nodesep=\""+std::to_string(maxHeight)+"\"];\n"
            + nodeString.str()
            + edgeString.str()
            + "\n}"
        ;
        // NE::log(dotString);
    }

    // compute layout
    {
        Agraph_t *G = agmemread(
            dotString.data()
        );
        GVC_t *gvc = gvContext();
        gvLayout(gvc, G, "dot");

        // read layout
        for(auto it : nodes){
            auto node = it.second;
            if(!node)
                continue;
            auto proxy = it.second->getProxy();
            if(!proxy)
                continue;

            auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
            if(proxyAsView)
                continue;

            Agnode_t *n = agnode(G, const_cast<char *>(std::to_string( NE::getID(proxy) ).data()), 0);
            if(n != nullptr) {
                auto &coord = ND_coord(n);
                node->setPos(
                    coord.x,
                    coord.y
                );
            }
        }

        // free memory
        gvFreeLayout(gvc, G);
        agclose(G);
        gvFreeContext(gvc);
    }

    // compute initial x position for all views
    std::vector<std::pair<Node*,qreal>> viewXMap;
    for(auto it : nodes){
        auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
        if(!proxyAsView){
            auto pos = it.second->pos();
            if(maxY<pos.y())
                maxY=pos.y();
            continue;
        }

        qreal avgX = 0;
        auto edgesIt = edges.find( NE::getID(proxyAsView) );
        if(edgesIt!=edges.end()){
            int nEdges = edgesIt->second.size();
            for(auto edge: edgesIt->second)
                avgX += edge->getProducer()->pos().x();
            avgX /= nEdges;
        }

        viewXMap.emplace_back(it.second,avgX);
    }

    // sort views by current x coord
    std::sort(
        viewXMap.begin(),
        viewXMap.end(),
        [](const std::pair<Node*,qreal>& a, const std::pair<Node*,qreal>& b){
            return a.second<b.second;
        }
    );

    // make sure all views have enough space
    qreal lastX = -1000.0;
    for(auto it: viewXMap){
        qreal width = it.first->boundingRect().width();

        qreal x = it.second;
        if(lastX+width>x)
            x = lastX+width + 10.0;

        it.first->setPos(
            x,
            maxY + maxHeight*100.0
        );

        lastX = x;
    }

    return 1;
#else
    NE::log("ERROR: GraphViz support disabled!",true);
    return 0;
#endif
}

QRect NE::Scene::getBoundingRect(std::unordered_map<int,NE::Node*>& nodes){
    int x0 = 999999;
    int x1 = -999999;
    int y0 = 999999;
    int y1 = -999999;

    for(auto it : nodes){
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

void NE::Scene::drawBackground(QPainter *painter, const QRectF &rect){
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