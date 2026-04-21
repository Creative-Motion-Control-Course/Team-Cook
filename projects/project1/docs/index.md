---
layout: default
title: "Interactive Moiré Pattern Plotter"
---

# Project 1: Interactive Moiré Pattern Plotter

## Concept

We envision an interactive CNC plotter system for creating animated moiré patterns through layered drawing and real-time user control. [Moiré patterns](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern#) arise when similar repetitive patterns overlap and shift relative to one another, producing compelling visual illusions of movement. Building on this principle, our system enables users to design and customize pattern types such as lines, grids, radial, and concentric forms, along with parameters like spacing, density, scaling, and rotation through an intuitive interface, then generates these patterns layer by layer using a plotter.

<p align = "left">
<img src = "https://upload.wikimedia.org/wikipedia/commons/thumb/c/cc/Moire.gif/250px-Moire.gif" height = 200 style = "margin-right:10px;">
<img src = "https://upload.wikimedia.org/wikipedia/commons/e/e3/070309-moire-a5-a5-upward-movement.gif" height = 200 style = "margin-right:10px;">
<img src = "https://upload.wikimedia.org/wikipedia/commons/0/08/070320-a6-shape-moire-pr-gt-pb.gif" height = 200 style = "margin-right:10px;">
</p>

The patterns will be printed on transparent films on a transparent platform. After the first layer is printed, it can be placed on a movable plate under the platform capable of translation or rotation. As the plotter continues to draw, the relative motion between these layers allows users to directly observe the emergence of animated moiré effects in real time. Additional layers can be added to increase visual complexity.

By integrating live parameter adjustment, physical motion, and diverse drawing media, the system transforms traditionally static geometric patterns into an embodied, real-time generative animation experience.

## Design

We began by defining the machine setup, the pattern types, the drawing media, and the key interaction parameters. We decided to first develop and test each pattern separately before combining them into layered compositions. In early experiments, we found that if two patterns are not similar enough, the moiré effect becomes weak or unclear. Based on this, we limited the range of user input to keep patterns within a controlled variation range.

#### Machine Design:
 <img src="assets/project 1 sketch.jpg" width="800">

#### Visual Output:
- Grids: lines, waves, dots
- Concentric patterns: circles, squares
- Radial patterns: spiral, radial lines, dots

#### Drawing Media:
- Pen
- Technical pen
- Brush
- Water color pen
- Transparent film papers / tracing paper

#### Interaction Design:
- Select a pattern type
- Adjust the number of lines
- Control wave generation - x,y of the pen: straight or curved lines
- Control wave generation - z of the pen: dots or short lines, stroke thickness
- Adjust movement speed
- Control spacing: absolute spacing, gradual changes (increasing or decreasing spacing over time)

## Implementation

We initially aimed to use the wave generators to draw spiral patterns. The system operates in polar coordinates, where both the angle and the radius are driven continuously. To control the motion, we use two velocity generators: one for the angle and one for the radius, and map the output through polar-to-Cartesian kinematics to drive the plotter. In this setup, the plotter first produces spiral patterns. By adding z-axis control to lift the pen at intervals and adjusting the relative speeds of the two velocity generators, we were able to segment the spiral and produce paths that visually similar to concentric circles. By further modifying the z movement, the system can also generate discrete concentric curves or even dots. For the interaction, users can adjust the spacing, wave scale, and line length.******

<img src="assets/spiral.jpg" width="800">

<img src="assets/concentric-circles.jpg" width="800">

We then worked on grid patterns. After learning the time-based interpolator (TBI), the implementation became much simpler. We used a for loop to generate evenly spaced lines to form the grid. By using the wave generator, the straight lines can be distorted. With additional z-axis control, the length of each line can vary, creating a lattice-like effect. One limitation we encountered is that in StepDance, the for loop can only support up to 25 iterations, which limits the resolution and density of the grid. For the interaction, users can adjust the spacing, wave scale, line count, and line length.******

<img src="assets/grids.jpg" width="800">

By defining x and y in the TBI to distribute points evenly around a center, we are able to generate radial lines. After adding the wave generator, we observed that the distortion mainly occurred along one direction, while the other direction appeared compressed, resulting in uneven patterns. To address this, we modified the TBI using a polar-coordinate approach*****, which provides more balanced control over angular and radial motion. For interaction, users can adjust the spiral angle and line length.

<img src="assets/radial-lines.jpg" width="800">

We finally worked on concentric square patterns. Using the TBI, we defined square paths and used a for loop to gradually increase the side length, generating multiple layers of squares from the center outward. Z-axis movement is used to lift the pen between each square. By adding a wave generator, the square edges can also be distorted. For interaction, users can adjust the spacing, wave scale, line count, and line length.******

<img src="assets/concentric-squares.jpg" width="800">

To combine everything, we organized each pattern into separate functions and used a button to toggle between different modes.******

### Hardware Setup

We used two buttons and four potentiometers (two sliders and two knobs) as input controls for adjusting system parameters. One button is used to switch between pattern types, while the other controls the start and stop of the machine.

![Hardware setup](assets/placeholder.jpg)

### Code Overview

#### Spiral / Radial Dots / Concentric Circles

```cpp
// Paste and explain relevant code snippets here
```

#### Grids

```cpp
// Paste and explain relevant code snippets here
```

#### Radial Lines/ Waves

```cpp
// Paste and explain relevant code snippets here
```

#### Concentric Squares

```cpp
// Paste and explain relevant code snippets here
```

## Results

<iframe width="560" height="315" src="https://www.youtube.com/embed/VIDEO_ID" frameborder="0" allowfullscreen></iframe>

<!-- *Replace the iframe above with your actual video URL, or use a local video:* -->

<!--
<video width="560" controls>
  <source src="assets/demo-video.mp4" type="video/mp4">
</video>
-->

Transparent platform display - to be continued...

## Reflection

We found working with polar coordinates especially interesting, as controlling angle and radius creates relative motion that feels intuitive yet produces complex and unexpected results. After learning TBI, we also developed a better understanding of how to construct motion sequences and how small adjustments in timing and parameters can lead to noticeable differences in the final output.******

Due to system limitations, we had to make several tradeoffs, such as simplifying pattern resolution and constraining parameter ranges to maintain stable results. To continue this project, we would refine the physical interface using 3D printing or laser cutting to create a more playful control device, consider a Raspberry Pi for better integration, and develop a tangible output such as a pull or rotating card so users can take the results with them.
