#include <NodeEditorView.h>

// qt includes
#include <QWheelEvent>

NodeEditorView::NodeEditorView(QWidget* parent)
    : QGraphicsView(parent){
};

NodeEditorView::NodeEditorView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent){
};

NodeEditorView::~NodeEditorView(){
};

void NodeEditorView::wheelEvent(QWheelEvent *event){
    const ViewportAnchor anchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    int angle = event->angleDelta().y();
    qreal factor;
    if (angle > 0) {
        factor = 1.1;
    } else {
        factor = 0.9;
    }
    scale(factor, factor);
    setTransformationAnchor(anchor);
};