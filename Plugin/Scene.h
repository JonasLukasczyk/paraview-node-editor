#pragma once

// qt includes
#include <QGraphicsScene>

// std includes
#include <unordered_map>

namespace NE {
    class Node;
    class Edge;
}

namespace NE {

    /// This class extends QGraphicsScene to
    /// * draw a grid background;
    /// * monitor the creation/modification/destruction of proxies to automatically
    ///   modify the scene accordingly;
    /// * manage the instances of nodes and edges;
    class Scene : public QGraphicsScene {
        Q_OBJECT

        public:
            Scene(QObject* parent=nullptr);
            ~Scene();

            QRect getBoundingRect(std::unordered_map<int,NE::Node*>& nodes);

        public slots:
            int computeLayout(
                std::unordered_map<int,NE::Node*>& nodes,
                std::unordered_map<int,std::vector<NE::Edge*>>& edges
            );

        protected:

            /// Draws a grid background.
            void drawBackground(QPainter *painter, const QRectF &rect);
    };

}