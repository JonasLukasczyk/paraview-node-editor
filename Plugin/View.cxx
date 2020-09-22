#include <View.h>

// qt includes
#include <QWheelEvent>
#include <QKeyEvent>
#include <QAction>
#include <pqDeleteReaction.h>

NE::View::View(QWidget* parent)
    : QGraphicsView(parent){
};

NE::View::View(QGraphicsScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , deleteAction(new QAction(this))
{
    // create delete reaction
    new pqDeleteReaction(this->deleteAction);

    this->setRenderHint(QPainter::Antialiasing);
};

NE::View::~View(){
};

void NE::View::wheelEvent(QWheelEvent *event){
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

void NE::View::keyReleaseEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Delete)
        this->deleteAction->trigger();

    return QWidget::keyReleaseEvent(event);
}