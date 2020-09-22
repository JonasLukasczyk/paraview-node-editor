#pragma once

// qt includes
#include <QGraphicsView>

// forward declarations
class QWheelEvent;
class QKeyEvent;
class QAction;

namespace NE {
    // This class extends QGraphicsView to rehandle MouseWheelEvents for zooming.
    class View : public QGraphicsView {
        public:

            View(QWidget* parent=nullptr);
            View(QGraphicsScene* scene, QWidget* parent=nullptr);
            ~View();

        protected:
            void wheelEvent(QWheelEvent *event);
            void keyReleaseEvent(QKeyEvent *event);

        private:
            QAction* deleteAction{nullptr};
    };
}
