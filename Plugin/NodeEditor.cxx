#include <NodeEditor.h>

// node editor includes
#include <NodeEditorScene.h>
#include <NodeEditorView.h>

// qt includes
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QEvent>

NodeEditor::NodeEditor(const QString &title, QWidget *parent) : QDockWidget(title, parent){
    // create inner widget
    QWidget* t_widget = new QWidget(this);

    // create layout
    auto layout = new QVBoxLayout;
    t_widget->setLayout(layout);

    // create node editor scene and view
    auto view = new NodeEditorView(
        new NodeEditorScene(this),
        this
    );
    view->setDragMode( QGraphicsView::ScrollHandDrag );
    view->setSceneRect(-10000,-10000,20000,20000);
    layout->addWidget(view);

    // set widget
    this->setWidget(t_widget);
}

NodeEditor::NodeEditor(QWidget *parent) : NodeEditor("Node Editor", parent){
}

NodeEditor::~NodeEditor(){
}