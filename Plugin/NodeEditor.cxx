#include <NodeEditor.h>

// node editor includes
#include <NodeEditorScene.h>
#include <NodeEditorView.h>

// qt includes
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QCheckBox>
#include <QEvent>
#include <iostream>

NodeEditor::NodeEditor(const QString &title, QWidget *parent) : QDockWidget(title, parent){
    // create inner widget
    auto t_widget = new QWidget(this);

    // create layout
    auto layout = new QVBoxLayout;
    t_widget->setLayout(layout);

    // create node editor scene and view
    auto scene = new NodeEditorScene(this);
    auto view = new NodeEditorView(
        scene,
        this
    );
    view->setDragMode( QGraphicsView::ScrollHandDrag );
    view->setSceneRect(-10000,-10000,20000,20000);
    layout->addWidget(view);

    auto computeLayoutAndAdjustZoom = [=](int force){
        if(!force && !this->autoUpdateLayout)
            return 1;

        scene->computeLayout();
        const int padding = 20;
        auto viewPort = scene->getBoundingRect();
        viewPort.adjust(-padding,-padding,padding,padding);
        view->fitInView(
            viewPort,
            Qt::KeepAspectRatio
        );
        return 1;
    };
    auto computeLayoutAndAdjustZoomII = [=](){
        return computeLayoutAndAdjustZoom(0);
    };
    auto computeLayoutAndAdjustZoomIII = [=](){
        return computeLayoutAndAdjustZoom(1);
    };

    this->connect(
        scene, &NodeEditorScene::nodesModified,
        this, computeLayoutAndAdjustZoomII
    );

    this->connect(
        scene, &NodeEditorScene::edgesModified,
        this, computeLayoutAndAdjustZoomII
    );

    auto toolbar = new QWidget(this);
    layout->addWidget(toolbar);

    auto toolbarLayout = new QHBoxLayout;
    toolbar->setLayout(toolbarLayout);

    auto layoutButton = new QPushButton("Layout");
    this->connect(
        layoutButton, &QPushButton::released,
        this, computeLayoutAndAdjustZoomIII
    );
    toolbarLayout->addWidget(layoutButton);

    auto autoLayoutCB = new QCheckBox("Automatic Layout");
    autoLayoutCB->setCheckState( Qt::Checked );
    this->connect(
        autoLayoutCB, &QCheckBox::stateChanged,
        this, [=](int state){
            this->autoUpdateLayout = state;
            return computeLayoutAndAdjustZoomIII();
        }
    );
    toolbarLayout->addWidget(autoLayoutCB);

    toolbarLayout->addItem( new QSpacerItem(0,0,QSizePolicy::Expanding) );

    // set widget
    this->setWidget(t_widget);
}

NodeEditor::NodeEditor(QWidget *parent) : NodeEditor("Node Editor", parent){
}

NodeEditor::~NodeEditor(){
}