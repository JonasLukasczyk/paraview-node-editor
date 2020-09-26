#pragma once

#include <QObject>

class QColor;
class pqProxy;

// forward declarations
namespace NE {

    void log(std::string content, bool force=false);
    int getID(pqProxy* proxy);
    std::string getLabel(pqProxy* proxy);

    namespace CONSTS {
        extern bool   DEBUG;
        extern int    NODE_WIDTH;
        extern int    NODE_PADDING;
        extern int    NODE_BORDER_WIDTH;
        extern int    NODE_BR_PADDING;
        extern int    EDGE_WIDTH;
        extern QColor COLOR_ORANGE;
        extern QColor COLOR_GREEN;
        extern double DOUBLE_CLICK_DELAY;
    };

    template<typename F>
    class Interceptor : public QObject {
        public:
            F functor;
            Interceptor(QObject* parent,F functor) : QObject(parent), functor(functor){}
            ~Interceptor(){
            }

            bool eventFilter(QObject *object, QEvent *event){
                return this->functor(object,event);
            }
    };

    template <typename F>
    Interceptor<F>* createInterceptor(QObject* parent, F functor) {
        return new Interceptor<F>(parent, functor);
    };

    double getTimeDelta();
    double getTimeStamp();

    bool isDoubleClick();
}

