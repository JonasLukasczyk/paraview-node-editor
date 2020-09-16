#pragma once

// QT includes
#include <QGraphicsPathItem>

// forward declarations
class Node;

/// Every instance of this class corresponds to an edge between an output port
/// and an input port. This class internally detects if the positions of the
/// corresponding ports change and updates itself automatically.
class Edge : public QObject, public QGraphicsPathItem {
    Q_OBJECT

    public:
        Edge(
            Node* producer,
            int producerOutputPortIdx,
            Node* consumer,
            int consumerInputPortIdx,
            QGraphicsItem *parent = nullptr
        );
        ~Edge();

        /// Print edge information.
        std::string print();

    protected:
        void updateEndPoints();
        QRectF boundingRect() const override;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    private:
        QPen* edgePen;
        QPointF oPoint;
        QPointF iPoint;

        Node* producer;
        int producerOutputPortIdx;
        Node* consumer;
        int consumerInputPortIdx;
};