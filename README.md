# paraview-node-editor

![screenshot](https://raw.githubusercontent.com/JonasLukasczyk/paraview-node-editor/master/screenshot.jpg "Screenshot of ParaView Node Editor")

### Overview
This repository contains a node editor plugin for ParaView. So far it is self-contained, but implementing some features (such as auto-update) will require some modifications of the ParaView source code. These modifications are on hold until the community reaches a consensuses on how to proceed with the architecture of the prototype.

### Installation
To install the plugin:
1. Build ParaView v5.8.0
2. Build NodeEditor in seperate folder (eventually you have to explicitly set ParaView_DIR with cmake)
3. Load the NodeEdior PlugIn in ParaView via "Tools/Manage PlugIns/Load New" and select NodeEditor.so located in paraview-node-editor/build/lib/paraview-5.8/plugins/NodeEditor
4. Add the NodeEditor to a dockable panel (it should be listed in the same menu as the "Pipeline Browser" and the "Properties" panel).

### Current Features
1. Automatically detects the creation/modification/destruction of source/filter/view proxies and manages nodes accordingly.
2. Automatically detects the creation/destruction of connections between ports and manages edges accordingly.
3. Every node exposes all properties of the corresponding proxy via the pqProxiesWidget class.
4. Property values are natively synchronized within other widgets, such as the ones shown in the properties panel.
5. Proxy selection is natively synchronized with the pipeline browser.
6. Seems to work with state files and python tracing.

### User Manual
* Filters/Views are selected by double-clicking their corresponding node labels (hold CTRL to select multiple filters).
* Output ports are selected by double-clicking their corresponding port labels (hold CTRL to select multiple ports).
* Nodes are collapsed/expanded by right-clicking node labels.
* The current active output port is set as the input of another filter by double-clicking the corresponding input port label.
* To toggle the visibility of an output port in the current active view SHIFT + left-click the corresponding output port (CTRL+SHIFT+left-click shows the output port exclusively)

### Current Limitations
1. Embedded property widgets that show a double input field are not shown correctly (integer inputs and even the calculator work). It looks like the problem comes from the pqDoubleLineEdit class.
2. Widgets that can show/hide an interactor in a view are currently not working.
3. Filters that require multiple inputs open upon creation a selection dialog for the inputs, which is no longer necessary in the node editor.
