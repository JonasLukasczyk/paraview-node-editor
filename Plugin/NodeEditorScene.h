#pragma once

// qt includes
#include <QGraphicsScene>
#include <unordered_map>

// forward declarations
class Node;
class Edge;
class pqProxy;
class pqPipelineSource;
class pqRepresentation;
class pqView;

/// This class extends QGraphicsScene to
/// * draw a grid background;
/// * monitor the creation/modification/destruction of proxies to automatically
///   modify the scene accordingly;
/// * manage the instances of nodes and edges;
class NodeEditorScene : public QGraphicsScene {
    Q_OBJECT

    public:
        NodeEditorScene(QObject* parent=nullptr);
        ~NodeEditorScene();

        int computeLayout();
        QRect getBoundingRect();

    protected:
        Node* createNode(pqProxy* proxy);
        int createNodeForSource(pqPipelineSource* proxy);
        int createNodeForView(pqView* proxy);
        int removeNode(pqProxy* proxy);

        int createEdges(pqPipelineSource *source, pqPipelineSource *consumer, int srcOutputPort);

        /// Draws a grid background.
        void drawBackground(QPainter *painter, const QRectF &rect);

    signals:
        void nodesModified();
        void edgesModified();

    private:
        /// The node registry stores a node for each proxy (currently ony source/filter proxies).
        /// The key is the global identifier of the node proxy.
        std::unordered_map<int,Node*> nodeRegistry;

        /// The edge registry stores all incoming edges of a node.
        /// The key is the global identifier of the node proxy.
        std::unordered_map<int,std::vector<Edge*>> edgeRegistry;
};