# paraview-node-editor

![screenshot](https://raw.githubusercontent.com/JonasLukasczyk/paraview-node-editor/master/screenshot.jpg "Screenshot of ParaView Node Editor")

### Overview
This repository contains a node editor plugin for ParaView. So far it is self-contained, but implementing some features (such as auto-update) will require some modifications of the ParaView source code. These modifications are on hold until the community reaches a consensuses on how to proceed with the architecture of the prototype.

### Installation
To install the plugin:
1. Clone ParaView v5.8.0
2. Put the NodeEditor repo into the paraview-git/Plugins folder
3. Configure the paraview build to also build the NodeEditor plugin
4. Run ParaView and add the NodeEditor to a dockable panel (it should be listed in the same menu as the "Pipeline Browser" and the "Properties" panel).

### What it already can do
1. Automatically detects the creation/modification/destruction of source/filter proxies and manages nodes accordingly.
2. Automatically detects the creation/destruction of connections between ports and manages edges accordingly.
3. Every node exposes all properties of the corresponding proxy via the pqProxiesWidget class.
4. Property values are natively synchronized within other widgets, such as the ones shown in the properties panel.
5. Proxy selection is natively synchronized with the pipeline browser.
6. Seems to work with state files and python tracing.

### Current Limitations
1. Embedded property widgets that show a double input field are not shown correctly (integer inputs and even the calculator work). It looks like the problem comes from the pqDoubleLineEdit class.
2. Does not use a graph layout engine to optimize the layout (currently nodes are just created in a row).
3. Only creates nodes for source/filter proxies.
4. Can not be used to control the visibility of source/filter proxies in a view.
5. Display properties are not shown in the nodes.
6. The pipeline update logic evolves around the apply-button of the default Properties panel, so although proxies are correctly marked as modified, one still has to interact with the properties panel to trigger a pipeline update.
