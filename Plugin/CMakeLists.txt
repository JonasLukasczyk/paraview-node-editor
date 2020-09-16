find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(interfaces)
set(sources
  Node.h
  Node.cxx
  Edge.h
  Edge.cxx
  NodeEditor.cxx
  NodeEditor.h
  NodeEditorView.cxx
  NodeEditorView.h
  NodeEditorScene.cxx
  NodeEditorScene.h
)

paraview_plugin_add_dock_window(
  CLASS_NAME NodeEditor
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(NodeEditor
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(NodeEditor
  PRIVATE
    Qt5::Widgets)
