#pragma once

// QT includes
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

// forward declarations
class pqProxy;
class pqProxyWidget;
class pqProxiesWidget;
class pqView;
class pqPipelineSource;
class pqDataRepresentation;
class pqServerManagerModel;
class QWidget;
class QGraphicsEllipseItem;
class Edge;

class Node : public QObject, public QGraphicsItem {
    Q_OBJECT

    public:

        Node(pqProxy* proxy, QGraphicsItem *parent = nullptr);

        /// Creates a node for a pqPipelineSource that consists of
        /// * an encapsulating rectangle
        /// * input and output ports
        /// * a widgetContainer for properties
        Node(pqPipelineSource* source, QGraphicsItem *parent = nullptr);

        /// TODO
        Node(pqView* view, QGraphicsItem *parent = nullptr);


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

        /// TODO
        int advanceVerbosity();

        /// Print node information.
        std::string print();

        // sets the type of the node (0:normal, 1: selected filter, 2: selected view)
        int setType(int type);
        int getType(){return this->type;};

        QRectF boundingRect() const override;

    signals:
        void nodeResized();
        void nodeMoved();
        void nodeClicked();

    protected:

        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        int addPort(int type, const int index, const QString& portLabel);

        void mousePressEvent(QGraphicsSceneMouseEvent * event);

    private:
        pqProxy* proxy;
        // pqProxiesWidget* proxyProperties;
        pqProxyWidget* proxyProperties;
        QWidget* widgetContainer;

        std::vector<QGraphicsEllipseItem*> iPorts;
        std::vector<QGraphicsEllipseItem*> oPorts;

        int type{0}; // 0: normal, 1: selected filter, 2: selected view
        int verbosity{0}; // 0: empty, 1: non-advanced, 2: advanced

        int labelHeight{30};
        int padding{4};
        int borderWidth{4};
        int width{300};
        // int portHeight{30};
        // int portRadius{10};
        int portHeight{24};
        int portRadius{8};
        int portContainerHeight{0};
};