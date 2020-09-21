#pragma once

// QT includes
#include <QGraphicsPathItem>

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
            std::string print();

        protected:
            void updatePoints();
            QRectF boundingRect() const override;
            void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        private:
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