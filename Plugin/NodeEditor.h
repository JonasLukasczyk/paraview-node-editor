#pragma once

#include <QDockWidget>

/// This is the root widget of the node editor that can be docked in ParaView.
/// It currently only contains the node editor canvas, but in the future one
/// can also add a toolbar.
class NodeEditor : public QDockWidget {
  Q_OBJECT

public:
  NodeEditor(QWidget *parent = nullptr);
  NodeEditor(const QString &title, QWidget *parent = nullptr);
  ~NodeEditor();

private:
  bool autoUpdateLayout{true};

};