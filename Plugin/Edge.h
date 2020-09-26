#pragma once

// QT includes
#include <QGraphicsPathItem>
#include <QGraphicsScene>

// forward declarations
namespace NE {
    class Node;
}

namespace NE {
    /// Every instance of this class corresponds to an edge between an output port
    /// and an input port. This class internally detects if the positions of the
    /// corresponding ports change and updates itself automatically.
    class Edge : public QObject, public QGraphicsPathItem {
        Q_OBJECT

        public:
            Edge(
                QGraphicsScene* scene,
                NE::Node* producer,
                int producerOutputPortIdx,
                NE::Node* consumer,
                int consumerInputPortIdx,
                int type = 0,
                QGraphicsItem *parent = nullptr
            );
            ~Edge();

            // sets the state of the edge (0:normal edge, 1: view edge)
            int setType(int type);
            int getType(){return this->type;};

            NE::Node* getProducer(){return this->producer;};
            NE::Node* getConsumer(){return this->consumer;};

            /// Print edge information.
            std::string toString();

        public slots:
            int updatePoints();

        protected:

            QRectF boundingRect() const override;
            void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        private:
            QGraphicsScene* scene;

            int type{0};
            QPointF oPoint;
            QPointF cPoint;
            QPointF iPoint;

            NE::Node* producer;
            int producerOutputPortIdx;
            NE::Node* consumer;
            int consumerInputPortIdx;
    };
}