#pragma once

// QT includes
#include <QGraphicsItem>

class QGraphicsEllipseItem;
class QGraphicsTextItem;

namespace NE {
    /// Every instance of this class corresponds to an edge between an output port
    /// and an input port. This class internally detects if the positions of the
    /// corresponding ports change and updates itself automatically.
    class Port : public QGraphicsItem {

        public:
            Port(
                int type,
                QString name = "",
                QGraphicsItem* parent=nullptr
            );
            ~Port();

            QGraphicsEllipseItem* getDisc(){
                return this->disc;
            }
            QGraphicsTextItem* getLabel(){
                return this->label;
            }

            int setStyle(int style);

        protected:
            QRectF boundingRect() const override;
            void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        private:
            QGraphicsEllipseItem* disc;
            QGraphicsTextItem* label;

            int borderWidth{4};
            int portRadius{8};
    };
}