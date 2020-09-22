#include <NodeEditor.h>

// node editor includes
#include <NodeEditorScene.h>
#include <NodeEditorView.h>
#include <Node.h>

// qt includes
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QCheckBox>
#include <QEvent>
#include <QAction>
#include <iostream>

// paraview includes
#include <pqProxy.h>
#include <pqProxyWidget.h>
#include <pqPipelineSource.h>
#include <pqView.h>

NodeEditor::NodeEditor(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
    , actionZoom(new QAction(this))
    , actionLayout(new QAction(this))
    , actionApply(new QAction(this))
    , actionReset(new QAction(this))
{
    // create inner widget
    auto t_widget = new QWidget(this);

    // create layout
    auto layout = new QVBoxLayout;
    t_widget->setLayout(layout);

    // create node editor scene and view
    this->scene = new NE::NodeEditorScene(this);
    this->view = new NE::NodeEditorView(
        this->scene,
        this
    );
    this->view->setDragMode( QGraphicsView::ScrollHandDrag );
    this->view->setSceneRect(-10000,-10000,20000,20000);
    layout->addWidget(this->view);

    QObject::connect(
        actionZoom,
        &QAction::triggered,
        this,
        [=](){
            const int padding = 20;
            auto viewPort = this->scene->getBoundingRect();
            viewPort.adjust(-padding,-padding,padding,padding);
            this->view->fitInView(
                viewPort,
                Qt::KeepAspectRatio
            );
            return 1;
        }
    );

    QObject::connect(
        actionLayout,
        &QAction::triggered,
        this,
        [=](){
            this->scene->computeLayout();
            return 1;
        }
    );

    QObject::connect(
        actionApply,
        &QAction::triggered,
        this,
        [=](){
            auto& nodes = this->scene->getNodeRegistry();
            for(auto it: nodes){
                auto proxy = dynamic_cast<pqPipelineSource*>(
                    it.second->getProxy()
                );
                if(proxy){
                    std::cout<<"apply "<<it.second->toString()<<std::endl;
                    it.second->getProxyProperties()->apply();
                    proxy->setModifiedState( pqProxy::ModifiedState::UNMODIFIED );
                }
            }
            for(auto it: nodes){
                auto proxy = dynamic_cast<pqPipelineSource*>(
                    it.second->getProxy()
                );
                if(proxy){
                    std::cout<<"update pipeline "<<it.second->toString()<<std::endl;
                    proxy->updatePipeline();
                }
            }
            for(auto it: nodes){
                auto proxy = dynamic_cast<pqView*>(
                    it.second->getProxy()
                );
                if(proxy){
                    std::cout<<"update view "<<it.second->toString()<<std::endl;
                    proxy->render();
                }
            }

            return 1;
        }
    );

    QObject::connect(
        actionReset,
        &QAction::triggered,
        this,
        [=](){
            auto& nodes = this->scene->getNodeRegistry();
            for(auto it: nodes){
                auto proxy = dynamic_cast<pqPipelineSource*>(
                    it.second->getProxy()
                );
                if(proxy){
                    std::cout<<"reset "<<it.second->toString()<<std::endl;
                    it.second->getProxyProperties()->reset();
                    proxy->setModifiedState( pqProxy::ModifiedState::UNMODIFIED );
                }
            }
            return 1;
        }
    );

    auto automaticListener = [=](){
        if(this->autoUpdateLayout)
            this->actionLayout->trigger();
        if(this->autoUpdateZoom)
            this->actionZoom->trigger();
        return 1;
    };

    this->connect(
        this->scene, &NE::NodeEditorScene::nodesModified,
        this, automaticListener
    );

    this->connect(
        this->scene, &NE::NodeEditorScene::edgesModified,
        this, automaticListener
    );

    auto toolbar = new QWidget(this);
    layout->addWidget(toolbar);

    auto toolbarLayout = new QHBoxLayout;
    toolbar->setLayout(toolbarLayout);

    auto addButton = [=](QString label, QAction* action){
        auto button = new QPushButton(label);
        this->connect(
            button, &QPushButton::released,
            action, &QAction::trigger
        );
        toolbarLayout->addWidget(button);
    };

    addButton("Apply", actionApply);
    addButton("Reset", actionReset);
    addButton("Layout", actionLayout);

    {
        auto checkBox = new QCheckBox("Auto Layout");
        checkBox->setCheckState( Qt::Checked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                this->autoUpdateLayout = state;
                automaticListener();
                return 1;
            }
        );
        toolbarLayout->addWidget(checkBox);
    }

    addButton("Zoom", actionZoom);

    {
        auto checkBox = new QCheckBox("Auto Zoom");
        checkBox->setCheckState( Qt::Checked );
        this->connect(
            checkBox, &QCheckBox::stateChanged,
            this, [=](int state){
                this->autoUpdateZoom = state;
                automaticListener();
                return 1;
            }
        );
        toolbarLayout->addWidget(checkBox);
    }

    // add spacer
    toolbarLayout->addItem( new QSpacerItem(0,0,QSizePolicy::Expanding) );

    // set widget
    this->setWidget(t_widget);
}

NodeEditor::NodeEditor(QWidget *parent) : NodeEditor("Node Editor", parent){
}

NodeEditor::~NodeEditor(){
}