#pragma once

// qt includes
#include <QDockWidget>

// std includes
#include <unordered_map>

// forward declarations
class QAction;
class QLayout;

class pqProxy;
class pqPipelineSource;
class pqRepresentation;
class pqOutputPort;
class pqView;

namespace NE {
    class Node;
    class Edge;
    class Scene;
    class View;
}

/// This is the root widget of the node editor that can be docked in ParaView.
/// It currently only contains the node editor canvas, but in the future one
/// can also add a toolbar.
class NodeEditor : public QDockWidget {
    Q_OBJECT

    public:
        NodeEditor(QWidget *parent = nullptr);
        NodeEditor(const QString &title, QWidget *parent = nullptr);
        ~NodeEditor();

    protected:
        NE::Node* createNode(pqProxy* proxy);

        int initializeActions();
        int createToolbar(QLayout* layout);
        int attachServerManagerListeners();

    public slots:
        int apply();
        int reset();
        int zoom();
        int layout();

        int createNodeForSource(pqPipelineSource* proxy);
        int createNodeForView(pqView* proxy);
        int removeNode(pqProxy* proxy);

        int setActivePortAsInput(pqPipelineSource *consumer, int idx);

        int updateActiveView();
        int updateActiveSourcesAndPorts();

        int removeIncomingEdges(pqProxy* proxy);
        int updatePipelineEdges(pqPipelineSource *consumer);
        int updateVisibilityEdges(pqView* proxy);

        int toggleInActiveView(pqOutputPort* port);
        int hideAllInActiveView();

        int collapseAllNodes();

    private:
        NE::Scene* scene;
        NE::View* view;

        bool autoUpdateLayout{true};
        QAction* actionZoom;
        QAction* actionLayout;
        QAction* actionApply;
        QAction* actionReset;
        QAction* actionAutoLayout;
        QAction* actionCollapseAllNodes;

        /// The node registry stores a node for each source/filter/view proxy
        /// The key is the global identifier of the node proxy.
        std::unordered_map<int,NE::Node*> nodeRegistry;

        /// The edge registry stores all incoming edges of a node.
        /// The key is the global identifier of the node proxy.
        std::unordered_map<int,std::vector<NE::Edge*>> edgeRegistry;
};
