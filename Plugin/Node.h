#pragma once

// QT includes
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

// forward declarations
class pqProxy;
class pqPipelineSource;
class pqDataRepresentation;
class pqServerManagerModel;
class QWidget;
class QGraphicsEllipseItem;
class Edge;

class Node : public QObject, public QGraphicsItem {
    Q_OBJECT

    public:
        /// Creates a node for a pqPipelineSource that consists of
        /// * an encapsulating rectangle
        /// * input and output ports
        /// * a widgetContainer for properties
        Node(pqPipelineSource* source, QGraphicsItem *parent = nullptr);

        /// Creates a node for a pqDataRepresentation that consists of
        /// * an encapsulating rectangle
        /// * one input port linked to a filter/source
        /// * one output port linked to a view
        /// * a widgetContainer for properties
        Node(pqDataRepresentation* representation, QGraphicsItem *parent = nullptr);

        /// Destructor
        ~Node();

        /// Delete copy constructor.
        Node(const Node&) =delete;
        /// Delete copy constructor.
        Node& operator=(const Node&) =delete;

        /// Get corresponding pqProxy of the node.
        pqProxy* getProxy(){
            return this->proxy;
        }

        /// Get input ports of the node.
        std::vector<QGraphicsEllipseItem*>& getInputPorts(){
            return this->iPorts;
        }

        /// Get output ports of the node.
        std::vector<QGraphicsEllipseItem*>& getOutputPorts(){
            return this->oPorts;
        }

        /// Get widget container of the node.
        QWidget* getWidgetContainer(){
            return this->widgetContainer;
        }

        /// Update the size of the node to fit its contents.
        int updateSize();

        /// Print node information.
        std::string print();

        // sets the state of the node (0:normal, 1: selected)
        int setState(int state);

    signals:
        void nodeMoved();
        void nodeClicked();

    protected:
        int initLabel();

        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        QRectF boundingRect() const override;

        int addPort(bool isInputPort, const int index, const QString& portLabel);

        void mousePressEvent(QGraphicsSceneMouseEvent * event);

    private:
        pqProxy* proxy;
        QWidget* widgetContainer;

        std::vector<QGraphicsEllipseItem*> iPorts;
        std::vector<QGraphicsEllipseItem*> oPorts;

        int state{0}; // 0: normal, 1: selected

        int labelHeight{35};
        int padding{4};
        int borderWidth{4};
        int width{300};
        int portHeight{30};
        int portRadius{10};
        int portContainerHeight{0};
};