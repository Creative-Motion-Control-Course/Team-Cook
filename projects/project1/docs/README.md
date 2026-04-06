# Project 1 - Proposal

We envision an interactive Moiré pattern plotter that enables live animation on moiré patterns during the CNC process.

[Moiré patterns](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern#) are visual interference patterns formed by layering similar patterns, which create the illusion of motion when there is relative movement between the layers. Using a live-controlled plotter, we enable real-time parameter adjustment, generating traditionally geometric Moiré patterns organically while supporting diverse drawing media.

<p align = "left">
<img src = "https://upload.wikimedia.org/wikipedia/commons/thumb/c/cc/Moire.gif/250px-Moire.gif" height = 200 style = "margin-right:10px;">
<img src = "https://upload.wikimedia.org/wikipedia/commons/e/e3/070309-moire-a5-a5-upward-movement.gif" height = 200 style = "margin-right:10px;">
<img src = "https://upload.wikimedia.org/wikipedia/commons/0/08/070320-a6-shape-moire-pr-gt-pb.gif" height = 200 style = "margin-right:10px;">
</p>

We want to design a system that allows the users to first personalize their preferred patterns through a simple online or physical (if time allows) interface, then let the plotter print them layer by layer. After the first layer is printed, it can be placed beneath a transparent moving plate that can translate or rotate. A second transparent plate above holds the printing surface. Through the relative motion between these layers, users can directly observe the animated effect during the second printing. Additional layers can also be added.

For the input, we plan to reference the online [Moiré Museum](https://www.sqrt.ch/museum) to design the parameters and define the types available for users to choose from. The pattern types can include line-based, grid-based, radial, and concentric patterns, with transformations (rotation, translation) and parameters (scaling, spacing, line thickness, density). Users can adjust them ideally on a physical interface with playful buttons and sliders that can preview the results in real-time.

This sketch describes the machine design:

 <img src="projects/project1/docs/assets/project 1 sketch.jpg" width="500">

These are some visual experiments we tried:

 <img src="projects/project1/docs/assets/experiment_1.gif" height = 200 style = "margin-right:10px;">
 <img src="projects/project1/docs/assets/experiment_2.gif" height = 200 style = "margin-right:10px;">
 <img src="projects/project1/docs/assets/experiment_3.gif" height = 200 style = "margin-right:10px;">
