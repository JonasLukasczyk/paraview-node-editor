#pragma once

// qt includes
#include <QGraphicsView>

// forward declarations
class QWheelEvent;
class QKeyEvent;
class QAction;

namespace NE {
    // This class extends QGraphicsView to rehandle MouseWheelEvents for zooming.
    class NodeEditorView : public QGraphicsView {
        public:

            NodeEditorView(QWidget* parent=nullptr);
            NodeEditorView(QGraphicsScene* scene, QWidget* parent=nullptr);
            ~NodeEditorView();

        protected:
            void wheelEvent(QWheelEvent *event);
            void keyReleaseEvent(QKeyEvent *event);

        private:
            QAction* deleteAction{nullptr};
    };
}
