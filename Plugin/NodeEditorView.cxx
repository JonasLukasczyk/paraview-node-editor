#include <NodeEditorView.h>

// qt includes
#include <QWheelEvent>
#include <QKeyEvent>
#include <QAction>
#include <pqDeleteReaction.h>

NE::NodeEditorView::NodeEditorView(QWidget* parent)
    : QGraphicsView(parent){
};

NE::NodeEditorView::NodeEditorView(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , deleteAction(new QAction(this))
{
    // create delete reaction
    new pqDeleteReaction(this->deleteAction);

    this->setRenderHint(QPainter::Antialiasing);
};

NE::NodeEditorView::~NodeEditorView(){
};

void NE::NodeEditorView::wheelEvent(QWheelEvent *event){
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

void NE::NodeEditorView::keyReleaseEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Delete)
        this->deleteAction->trigger();

    return QWidget::keyReleaseEvent(event);
}