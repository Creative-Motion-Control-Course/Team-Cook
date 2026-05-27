/*
Project 1 - combined patterns final version

serial commands:
1). change drawing area (grid, square, radial):
{"name": "set_drawing_area", "args": [40]}

* radial pattern's drawing area can also be changed by analog_1

2). pattern mode selection
0 - grid; 1 - centric ciecle; 2 - centric square; 3- radians
{"name": "set_pattern_mode", "args": [0]}

3). drawing velocity (grid, square, radial)
{"name": "set_drawing_velocity", "args": [8]}

* range: 0-100; recommended range: 5-40

4). set basic x&y wave parameters (grid, square)
{"name": "set_basic_wave", "args": [4, 3, 2, 1]}

* f_x, a_x, f_y, a_y (amplitude do not exceed 5, frequency do not exceed 5)

5). basic spacing between circles (circle)
{"name": "set_basic_spacing", "args": [0.1]}

6). spacing variation wave (circle)
{"name": "set_spacing_variation", "args": [0.2, 0.3]}

* [frequency, amplitude]

======================================
change pattern by serial communication
button_1 for enabling & disabling motor
button_2 for start pattern drawing

======================================
1) grid pattern
drawing grid or parallel lines
Before drawing, user can set the number of lines (analog_1), basic oscillation frequency & amplitude (serial), drawing area (serial) and velocity (serial).
During drawing, user can vary the oscallation scaling factor in amplitude and frequency (analog_3, analog_4), 
and pen up and down movement frequency to achieve varied line length(analog_2)

push the button_2 to draw vertical line; push button_2 again to draw horizontal lines

analog input: z-wave frequency, line count (4-25)
serial input: x, y wave basic amplitude & frequency, drawing velocity
button_2: start drawing vertical/horizontal lines + auto homing


2) centric circle (spiral)
Drawing spiral lines in polar coordinates
The circles have a basic spacing, which can be set before drawing by serial command anytime.
They also have a spacing variation, making the spacing change regularly, which can also be set through serial command anytime.
The spiral lines have oscillations, whose amplitude and frequency can be controlled by analog_1 and analog_3
The circle's drawing speed can be adjusted by analog_4 anytime

Push button_2, the pen go back to the starting point. 
*NOTE: this button homing function is not finished, currently after the pen goes back to the origin, it immediately continues drawing again


3) centric square
draw centric squares
The squares are drawn in vertical lines and horizontal lines separately, by pushing button_2
Before drawing, user can change the number of lines they are going to draw(analog_1), basic oscillation frequency & amplitude (serial), drawing area (serial) and velocity (serial)
During drawing, user can vary the oscillation scaling factor in amplitude and frequency (analog_3, analog_4), 
and pen up and down movement frequency to achieve varied line length(analog_2)


4) Radial pattern
Draw radial lines, these lines can be straight or curved
Before drawing, user can change the number of lines they are going to draw(analog_1), drawing area or line length (analog_4), angle deviation for each line (analog_3), and drawing speed(serial)
and pen up and down movement frequency to achieve varied line length(analog_2)

push button_2 to start drawing a sequence of lines

==============================
To be solved:
TBI conflict with z-wave motion
button homing function for centric circles


*/


#define module_driver   // tells compiler we're using the Stepdance Driver Module PCB
                        // This configures pin assignments for the Teensy 4.1

#include "stepdance.hpp"  // Import the stepdance library

int pattern_mode = 3; //0 - grid; 1 - centric ciecle; 2 - centric square; 4- radians

int drawing_area = 60; //grid's drawing area


int spacing; //grid
int line_count; //grid
int line_length; //grid

int xy_wave_scaler_a = 1; //grid + square
int xy_wave_scaler_f = 1; //grid + square

int square_count; //square
int flag_v; //square
int flag_h; //square

int basic_wave_x_f; //grid + square
int basic_wave_x_a; //grid + square
int basic_wave_y_f; //grid + square
int basic_wave_y_a; //grid + square

int drawing_velocity = 10.0; //grid, square

int radial_lines = 20; //radial
float radial_length; //radial line length, drawing area/2
float deviation;

// -- Define Input Ports --
InputPort input_a;

// -- Define Output Ports --
// Output ports generate step and direction electrical signals
// Here, we control two stepper drivers and a servo driver
// We choose names that match the labels on the PCB

OutputPort output_a;  // Axidraw left motor
OutputPort output_b;  // Axidraw right motor
OutputPort output_c;  // Z axis, a servo driver for the AxiDraw

// -- Define Motion Channels --
// Channels track target positions and interface with output ports
// Generally, we need a channel for each output port
// We choose names that match the axes of the AxiDraw's motors

Channel channel_a;  //AxiDraw "A" axis --> left motor motion
Channel channel_b;  // AxiDraw "B" axis --> right motor motion
Channel channel_z;  // AxiDraw "Z" axis --> pen up/down

// -- Define Kinematics --
// Kinematics convert between two coordinate spaces.
// We think in XY, but the axidraw moves in AB according to "CoreXY" (also "HBot") kinematics
KinematicsCoreXY axidraw_kinematics;
KinematicsPolarToCartesian polar_kinematics;

// -- Define Encoders --
// Encoders read quadrature input signals and we can map the signal to a channel or other elements
Encoder encoder_1;
Encoder encoder_2;

AnalogInput analog_a1; //grid line_count. // 
AnalogInput analog_a2; //wave frequency - z
AnalogInput analog_a3; // x y wave amplitude scaler
AnalogInput analog_a4; // x y wave frequency scaler

// -- Define Input Button --
Button button_d1;
Button button_d2;

// -- RPC Interface --
RPC rpc;

// -- Wave Generator --
//1) grid
WaveGenerator1D wave_gen_x;
WaveGenerator1D wave_gen_y;
WaveGenerator1D wave_gen_z;
//2) centric circle
//WaveGenerator1D wave_gen_z;
WaveGenerator1D wave_gen_r;
WaveGenerator1D wave_gen_s;

WaveGenerator1D wave_gen_d;

// -- Velocity Generator --
//2) centric circle
VelocityGenerator velocity_gen_r;
VelocityGenerator velocity_gen_a;

PositionGenerator position_gen_r;

TimeBasedInterpolator time_based_interpolator;


void setup() {
  //---------------------universal setups------------------------------
  // -- Configure and start the output ports --
  output_a.begin(OUTPUT_A); // "OUTPUT_A" specifies the physical port on the PCB for the output.
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);

  // Enable the output drivers
  enable_drivers();

  // -- Configure and start the channels --
  channel_a.begin(&output_a, SIGNAL_E);
  channel_b.begin(&output_b, SIGNAL_E);

  // These ratios are for the Axidraw V3: 2032 steps correspond to 1 inch (25.4mm)
  channel_a.set_ratio(25.4, 2032);
  channel_a.invert_output(); // We do that so that the X axis points from motor A to motor B (left to right)
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output(); // We do that so that the Y axis points down (away from the long axis)

  channel_z.begin(&output_c, SIGNAL_E);
  channel_z.set_ratio(1, 50); //straight step pass-thru.

  rpc.begin(); 

  //-------------------------------shared configurations---------------------------------------

  encoder_1.begin(ENCODER_1); // "ENCODER_1" specifies the physical port on the PCB
  encoder_1.set_ratio(24, 2400);  // 24mm per revolution, where 1 rev == 2400 encoder pulses
  encoder_1.output.map(&axidraw_kinematics.input_x);

  encoder_2.begin(ENCODER_2); // "ENCODER_2" specifies the physical port on the PCB
  encoder_2.set_ratio(24, 2400);  // 24mm per revolution, where 1 rev == 2400 encoder pulses
  encoder_2.output.map(&axidraw_kinematics.input_y);

  wave_gen_z.setNoInput();    
  wave_gen_z.frequency = 0.0; 
  wave_gen_z.amplitude = 3.0; 
  wave_gen_z.output.map(&channel_z.input_target_position);
  wave_gen_z.begin();

  polar_kinematics.output_x.map(&axidraw_kinematics.input_x);
  polar_kinematics.output_y.map(&axidraw_kinematics.input_y);
  polar_kinematics.begin();

  axidraw_kinematics.begin();
  axidraw_kinematics.output_a.map(&channel_a.input_target_position);
  axidraw_kinematics.output_b.map(&channel_b.input_target_position);

  button_d1.begin(IO_D1, INPUT_PULLDOWN);
  button_d1.set_mode(BUTTON_MODE_TOGGLE);

  button_d2.begin(IO_D2, INPUT_PULLDOWN);
  button_d2.set_mode(BUTTON_MODE_TOGGLE);
  button_d2.set_callback_on_press(&button_on_press);
  button_d2.set_callback_on_release(&button_on_release);

  analog_a1.begin(IO_A1);
  analog_a2.begin(IO_A2);
  analog_a3.begin(IO_A3);
  analog_a4.begin(IO_A4);

  draw_grid_setup();
  draw_circle_setup();
  draw_square_setup();
  draw_radial_setup();

//---------------------------rpc------------------------------------------------

  //{"name": "hello"}
  // expected result: serial monitor prints "hello!{"result":"ok"}"
  rpc.enroll("hello", hello_serial);

  //drawing area for square and grid
  //{"name": "set_drawing_area", "args": [40]}
  rpc.enroll("set_drawing_area", set_drawing_area);

  //pattern selection 0 - grid; 1 - centric ciecle; 2 - centric square; 3- radians
  //{"name": "set_pattern_mode", "args": [0]}
  rpc.enroll("set_pattern_mode", set_pattern_mode);

  //{"name": "set_basic_wave", "args": [4, 3, 2, 1]}
  //f_x, a_x, f_y, a_y (amplitude do not exceed 5, frequency do not exceed 5)
  rpc.enroll("set_basic_wave", set_basic_wave);

  //{"name": "set_drawing_velocity", "args": [8]}
  rpc.enroll("set_drawing_velocity", set_drawing_velocity);

  //{"name": "set_basic_spacing", "args": [0.1]}
  rpc.enroll("set_basic_spacing", set_basic_spacing);

  //{"name": "set_spacing_variation", "args": [0.2, 0.3]}
  //frequency, amplitude
  rpc.enroll("set_spacing_variation", set_spacing_variation);


//--------------universal functions---------------

  // --TBI--
  time_based_interpolator.begin();
  time_based_interpolator.output_z.map(&channel_z.input_target_position);

  // -- Start the stepdance library --
  dance_start();
}

LoopDelay overhead_delay;

void loop() {
  overhead_delay.periodic_call(&report_overhead, 500);

if(pattern_mode == 0){
  draw_grid_loop();
}

if(pattern_mode == 1){
  draw_circle_loop();
}

if(pattern_mode == 2){
  draw_square_loop();
}

if(pattern_mode == 3){
  draw_radial_loop();
}


if (button_d1.read_raw()) {
  motors_enable();
} else {
  motors_disable();
}

  dance_loop();

}



void draw_grid_horizontal() {

  for(int i = 0; i <line_count-1; i++){
  // mode, vel, x, y, z, 0, 0, 0
  // pen up, move to horizontal line starting point
  time_based_interpolator.add_move(GLOBAL, 50.0, -line_length / 2, -i * spacing + line_length / 2, 3, 0, 0, 0); 

  //pen down
  time_based_interpolator.add_move(GLOBAL, 50.0, -line_length / 2, -i * spacing + line_length / 2, -3, 0, 0, 0); 

  //draw a horizontal line
  time_based_interpolator.add_move(GLOBAL, drawing_velocity, line_length / 2, -i * spacing + line_length / 2, -3, 0, 0, 0); 

  //pen up
  time_based_interpolator.add_move(GLOBAL, 50.0, line_length / 2, -i * spacing + line_length / 2, 3, 0, 0, 0); 

  }

  //go to home
  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 0, 0, 0, 0); // go to home after drawing

}


void draw_grid_vertical() {

  for(int i = 0; i <line_count-1; i++){
  // mode, vel, x, y, z, 0, 0, 0
  //pen up, move to the vertical line starting point
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, -line_length / 2, 3, 0, 0, 0); 

  //pen down
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, -line_length / 2, -3, 0, 0, 0); 

  //draw a vertical line
  time_based_interpolator.add_move(GLOBAL, drawing_velocity, -i * spacing + line_length / 2, line_length / 2, -3, 0, 0, 0); 

  //pen up
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, line_length / 2, 3, 0, 0, 0); 

  }

//go to home
  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 0, 0, 0, 0);  //go to home after drawing 

}

void start_circle_drawing(){
  
  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, -4, 0, 0, 0); 
  velocity_gen_r.speed_units_per_sec = 0.15; 
  wave_gen_s.frequency = 0.1;
  wave_gen_s.amplitude = 0.2;

}



void go_to_home(){ //not resolved!!!!!!
  
  velocity_gen_r.speed_units_per_sec = 0.0; 
  velocity_gen_a.speed_units_per_sec = 0.0; 
  wave_gen_s.frequency = 0;
  wave_gen_s.amplitude = 0;

  //position_gen_r.go(0, ABSOLUTE, 100);

  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 4, 0, 0, 0); 
  

}



void draw_squares_vertical() {
  
  for (int i = 1; i <= square_count; i++) {
    float half = i * drawing_area / (square_count * 2.0);
    if (flag_v == 0){  
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, +half, 4, 0, 0, 0);
    // pen down draw a line
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, -half, -4, 0, 0, 0);
    //pen up
    time_based_interpolator.add_move(GLOBAL, 80, +half, -half, 4, 0, 0, 0);

    }
    else{
    // pen up, move to the drawing starting point
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, -half, 4, 0, 0, 0);
    // pen down draw a line
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, +half, -4, 0, 0, 0);
    //pen up
    time_based_interpolator.add_move(GLOBAL, 80, -half, +half, 4, 0, 0, 0);
    }
  }

   if(flag_v == 0){
    flag_v = 1;
  }
  else{
    flag_v = 0;
  }

  //go to home
  time_based_interpolator.add_move(GLOBAL, 20.0, 0, 0, 0, 0, 0, 0); 

}

void draw_squares_horizontal() {
  float vel = 20.0;
  
  for (int i = 1; i <= square_count; i++) {
    float half = i * drawing_area / (square_count * 2.0);
    if (flag_h == 0){  

    time_based_interpolator.add_move(GLOBAL, vel, +half, +half, 4, 0, 0, 0);
    // pen down draw a line
    time_based_interpolator.add_move(GLOBAL, vel, +half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, -half, +half, -4, 0, 0, 0);
    //pen up
    time_based_interpolator.add_move(GLOBAL, 80, -half, +half, 4, 0, 0, 0);
    }
    else{
    // pen up, move to the drawing starting point
    time_based_interpolator.add_move(GLOBAL, vel, -half, -half, 4, 0, 0, 0);
    // pen down draw a line
    time_based_interpolator.add_move(GLOBAL, vel, -half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, +half, -half, -4, 0, 0, 0);
    //pen up
    time_based_interpolator.add_move(GLOBAL, 80, +half, -half, 4, 0, 0, 0);
    }
  }

  if(flag_h == 0){
    flag_h = 1;
  }
  else{
    flag_h = 0;
  }

  //go to home
  time_based_interpolator.add_move(GLOBAL, 20.0, 0, 0, 0, 0, 0, 0); 

}


void draw_radial() {

  radial_length = drawing_area / 2;

  for (int i = 0; i < radial_lines; i++) {
    //pen up, move to origin
    time_based_interpolator.add_move(GLOBAL, 20, 0, 2 * PI * i / radial_lines, 4, 0, 0, 0);
    //pen down
    time_based_interpolator.add_move(GLOBAL, 50, 0, 2 * PI * i / radial_lines, -4, 0, 0, 0);
    //draw line
    //time_based_interpolator.add_move(GLOBAL, drawing_velocity, radial_length * cos(2 * PI * i / radial_lines), radial_length * sin(2 * PI * i / radial_lines), -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, radial_length, 2 * PI * i / radial_lines + deviation * PI, -4, 0, 0, 0);
    //pen up
    time_based_interpolator.add_move(GLOBAL, 80, radial_length, 2 * PI * i / radial_lines + deviation * PI, 4, 0, 0, 0);
  }

  //go to home
  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 2 * PI, 0, 0, 0, 0); 

}



//=========================grid======================
void draw_grid_setup(){

  wave_gen_x.setNoInput(); 
  wave_gen_x.frequency = 0.0; 
  wave_gen_x.amplitude = 0.0; 
  wave_gen_x.begin();

  wave_gen_y.setNoInput(); 
  wave_gen_y.frequency = 0.0; 
  wave_gen_y.amplitude = 0.0;
  wave_gen_y.begin();


}


void draw_grid_loop(){
  time_based_interpolator.output_x.map(&axidraw_kinematics.input_x);
  time_based_interpolator.output_y.map(&axidraw_kinematics.input_y);

  wave_gen_x.output.map(&axidraw_kinematics.input_x);
  wave_gen_y.output.map(&axidraw_kinematics.input_y);

  //line_count
  analog_a1.set_floor(4, 25);
  analog_a1.set_ceiling(25, 1020); //memory limit: 25 motion
  
  //z- wave frequency
  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);
  analog_a2.map(&wave_gen_z.frequency);

  //x y wave scaler amplitude
  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(3, 1020);

  //x y wave scaler frequency
  analog_a4.set_floor(0, 25);
  analog_a4.set_ceiling(4, 1020);
  
  line_count = analog_a1.read();
  spacing= drawing_area / line_count;
  line_length = spacing * (line_count - 1);
  //Serial.println(spacing);
  //Serial.println(line_count);
  //Serial.println(line_length); 

  xy_wave_scaler_a = analog_a3.read();
  xy_wave_scaler_f = analog_a4.read();

  wave_gen_x.frequency = basic_wave_x_f * xy_wave_scaler_f; 
  wave_gen_x.amplitude = basic_wave_x_a * xy_wave_scaler_a; 
  wave_gen_y.frequency = basic_wave_y_f * xy_wave_scaler_f; 
  wave_gen_y.amplitude = basic_wave_y_a * xy_wave_scaler_a; 

  // Serial.println(xy_wave_scaler_a);
  // Serial.println(wave_gen_x.amplitude);
  polar_kinematics.disable();
}

//=========================circle==========================

void draw_circle_setup(){

  position_gen_r.output.map(&polar_kinematics.input_radius);
  position_gen_r.begin();

  //wave radius variation
  wave_gen_r.setNoInput(); 
  wave_gen_r.frequency = 0; //analog_a2
  wave_gen_r.amplitude = 0.6; //analog_a3
  wave_gen_r.begin();

  //ciecle spacing variation - serial command
  wave_gen_s.setNoInput(); 
  wave_gen_s.frequency = 0.1;
  wave_gen_s.amplitude = 0.2;
  wave_gen_s.begin();

  //circle spacing constant
  velocity_gen_r.begin();
  velocity_gen_r.speed_units_per_sec = 0.15; //basic circle spacing - serial command

  //turning speed constant
  velocity_gen_a.begin();
  velocity_gen_a.speed_units_per_sec = 0.8; //basic turning speed - analog_a4

}


void draw_circle_loop(){
  polar_kinematics.enable();

  wave_gen_r.output.map(&polar_kinematics.input_radius);
  wave_gen_s.output.map(&polar_kinematics.input_radius);
  velocity_gen_r.output.map(&polar_kinematics.input_radius);
  velocity_gen_a.output.map(&polar_kinematics.input_angle);

  //z axis pen movement frequency
  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0.0, 1020); 
  analog_a2.map(&wave_gen_z.frequency);

  analog_a1.set_floor(0.0, 25);
  analog_a1.set_ceiling(15.0, 1020); 
  analog_a1.map(&wave_gen_r.frequency);

  analog_a3.set_floor(0.0, 25);
  analog_a3.set_ceiling(15.0, 1020); 
  analog_a3.map(&wave_gen_r.amplitude);

  analog_a4.set_floor(0.0, 25);
  analog_a4.set_ceiling(2.0, 1020); 
  analog_a4.map(&velocity_gen_a.speed_units_per_sec);

}

//=====================square==========================
void draw_square_setup(){
  draw_grid_setup();
}

void draw_square_loop(){

  time_based_interpolator.output_x.map(&axidraw_kinematics.input_x);
  time_based_interpolator.output_y.map(&axidraw_kinematics.input_y);

  wave_gen_x.output.map(&axidraw_kinematics.input_x);
  wave_gen_y.output.map(&axidraw_kinematics.input_y);

  //square_count
  analog_a1.set_floor(4, 25);
  analog_a1.set_ceiling(24, 1020);
  square_count = analog_a1.read();

  //z- wave frequency
  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);//15
  analog_a2.map(&wave_gen_z.frequency);

  //x y wave scaler amplitude
  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(3, 1020);

  //x y wave scaler frequency
  analog_a4.set_floor(0, 25);
  analog_a4.set_ceiling(4, 1020);

  xy_wave_scaler_a = analog_a3.read();
  xy_wave_scaler_f = analog_a4.read();

  wave_gen_x.frequency = basic_wave_x_f * xy_wave_scaler_f; 
  wave_gen_x.amplitude = basic_wave_x_a * xy_wave_scaler_a; 
  wave_gen_y.frequency = basic_wave_y_f * xy_wave_scaler_f; 
  wave_gen_y.amplitude = basic_wave_y_a * xy_wave_scaler_a; 

  polar_kinematics.disable();
}


//=====================radial=============================
void draw_radial_setup(){

  // wave_gen_d.setNoInput(); 
  // wave_gen_d.frequency = 0.0; 
  // wave_gen_d.amplitude = 0.0;
  // wave_gen_d.begin();

}

void draw_radial_loop(){
  time_based_interpolator.output_x.map(&polar_kinematics.input_radius);
  time_based_interpolator.output_y.map(&polar_kinematics.input_angle);

  //line_count
  analog_a1.set_floor(3, 25);
  analog_a1.set_ceiling(25, 1020);
  radial_lines = analog_a1.read();

  //z- wave frequency
  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);//15
  analog_a2.map(&wave_gen_z.frequency);

  //angle deviation
  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(1.5, 1020);
  deviation = analog_a3.read();

  analog_a4.set_floor(10, 25);
  analog_a4.set_ceiling(150, 1020);
  drawing_area = analog_a4.read();
  
}


//====================button functions=================

void button_on_press(){

if(pattern_mode == 0){ //grid
  draw_grid_vertical();
}
if(pattern_mode == 1){ //centric circle
  start_circle_drawing();
}

if(pattern_mode == 2){ //centric squares
  draw_squares_vertical();
}

if(pattern_mode == 3){ //radial
  draw_radial();
}

}


void button_on_release(){

if(pattern_mode == 0){ //grid
  draw_grid_horizontal();
}
if(pattern_mode == 1){ //centric circle
  go_to_home();
}
if(pattern_mode == 2){ //square
  draw_squares_horizontal();
}
if(pattern_mode == 3){ //radial
  draw_radial();
}

}


void motors_enable(){
  enable_drivers();
}

void motors_disable(){
  disable_drivers();
}


//==============================rpc functions=============================

//grid + square + radial
void set_drawing_velocity(float32_t velocity){
  drawing_velocity = velocity;
}

//grid + square
void set_basic_wave(float32_t frequency_x, float32_t amplitude_x, float32_t frequency_y, float32_t amplitude_y){

basic_wave_x_f = frequency_x;
basic_wave_x_a = amplitude_x;
basic_wave_y_f = frequency_y;
basic_wave_y_a = amplitude_y;

}

//circle
void set_basic_spacing(float32_t spacing){
  velocity_gen_r.speed_units_per_sec = spacing;
}

//circle
void set_spacing_variation(float32_t frequency, float32_t amplitude){
  wave_gen_s.frequency = frequency;
  wave_gen_s.amplitude = amplitude;
}

//general rpc

void set_pattern_mode(float32_t mode){
  pattern_mode = mode;
}

void set_drawing_area(float32_t area){
 drawing_area = area;
}

void hello_serial(){
  Serial.print("hello!");
}

//============================other functions=============================


void report_overhead(){

}
