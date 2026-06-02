/*
Project 1 - combined patterns final version
  + state reporting for the "interference" preview interface

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
STATE REPORTING (added for preview interface):
The board now prints its live state ~10x/sec as a JSON line:
  {"a":[a1,a2,a3,a4],"mode":M,"motor":0/1}
and prints button_2 events:
  {"btn2":"press"} / {"btn2":"release"}
The preview interface reads these lines to mirror the physical board.

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
OutputPort output_a;  // Axidraw left motor
OutputPort output_b;  // Axidraw right motor
OutputPort output_c;  // Z axis, a servo driver for the AxiDraw

// -- Define Motion Channels --
Channel channel_a;  //AxiDraw "A" axis --> left motor motion
Channel channel_b;  // AxiDraw "B" axis --> right motor motion
Channel channel_z;  // AxiDraw "Z" axis --> pen up/down

// -- Define Kinematics --
KinematicsCoreXY axidraw_kinematics;
KinematicsPolarToCartesian polar_kinematics;

// -- Define Encoders --
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
WaveGenerator1D wave_gen_r;
WaveGenerator1D wave_gen_s;

WaveGenerator1D wave_gen_d;

// -- Velocity Generator --
//2) centric circle
VelocityGenerator velocity_gen_r;
VelocityGenerator velocity_gen_a;

PositionGenerator position_gen_r;

TimeBasedInterpolator time_based_interpolator;

// ====== ADDED: live values cached each loop so the reporter can read them ======
float live_a1 = 0, live_a2 = 0, live_a3 = 0, live_a4 = 0;


void setup() {

  Serial.begin(115200);

  //---------------------universal setups------------------------------
  output_a.begin(OUTPUT_A);
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);

  enable_drivers();

  channel_a.begin(&output_a, SIGNAL_E);
  channel_b.begin(&output_b, SIGNAL_E);

  channel_a.set_ratio(25.4, 2032);
  channel_a.invert_output();
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output();

  channel_z.begin(&output_c, SIGNAL_E);
  channel_z.set_ratio(1, 50);

  rpc.begin();

  //-------------------------------shared configurations---------------------------------------
  encoder_1.begin(ENCODER_1);
  encoder_1.set_ratio(24, 2400);
  encoder_1.output.map(&axidraw_kinematics.input_x);

  encoder_2.begin(ENCODER_2);
  encoder_2.set_ratio(24, 2400);
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
  rpc.enroll("hello", hello_serial);
  rpc.enroll("set_drawing_area", set_drawing_area);
  rpc.enroll("set_pattern_mode", set_pattern_mode);
  rpc.enroll("set_basic_wave", set_basic_wave);
  rpc.enroll("set_drawing_velocity", set_drawing_velocity);
  rpc.enroll("set_basic_spacing", set_basic_spacing);
  rpc.enroll("set_spacing_variation", set_spacing_variation);

//--------------universal functions---------------
  time_based_interpolator.begin();
  time_based_interpolator.output_z.map(&channel_z.input_target_position);

  dance_start();
}

LoopDelay overhead_delay;
LoopDelay report_delay;   // ====== add report ======


void loop() {

  overhead_delay.periodic_call(&report_overhead, 500);
  report_delay.periodic_call(&report_state, 100);

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
  time_based_interpolator.add_move(GLOBAL, 50.0, -line_length / 2, -i * spacing + line_length / 2, 3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, 50.0, -line_length / 2, -i * spacing + line_length / 2, -3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, drawing_velocity, line_length / 2, -i * spacing + line_length / 2, -3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, 50.0, line_length / 2, -i * spacing + line_length / 2, 3, 0, 0, 0);
  }

  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 0, 0, 0, 0);
}


void draw_grid_vertical() {

  for(int i = 0; i <line_count-1; i++){
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, -line_length / 2, 3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, -line_length / 2, -3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, drawing_velocity, -i * spacing + line_length / 2, line_length / 2, -3, 0, 0, 0);
  time_based_interpolator.add_move(GLOBAL, 50.0, -i * spacing + line_length / 2, line_length / 2, 3, 0, 0, 0);
  }

  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 0, 0, 0, 0);
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

  time_based_interpolator.add_move(GLOBAL, 50.0, 0, 0, 4, 0, 0, 0);
}


void draw_squares_vertical() {

  for (int i = 1; i <= square_count; i++) {
    float half = i * drawing_area / (square_count * 2.0);
    if (flag_v == 0){
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, +half, 4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, +half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 80, +half, -half, 4, 0, 0, 0);
    }
    else{
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, -half, 4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, -half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 80, -half, +half, 4, 0, 0, 0);
    }
  }

   if(flag_v == 0){
    flag_v = 1;
  }
  else{
    flag_v = 0;
  }

  time_based_interpolator.add_move(GLOBAL, 20.0, 0, 0, 0, 0, 0, 0);
}


void draw_squares_horizontal() {
  float vel = 20.0;

  for (int i = 1; i <= square_count; i++) {
    float half = i * drawing_area / (square_count * 2.0);
    if (flag_h == 0){

    time_based_interpolator.add_move(GLOBAL, vel, +half, +half, 4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, +half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, -half, +half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 80, -half, +half, 4, 0, 0, 0);
    }
    else{
    time_based_interpolator.add_move(GLOBAL, vel, -half, -half, 4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, -half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, vel, +half, -half, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 80, +half, -half, 4, 0, 0, 0);
    }
  }

  if(flag_h == 0){
    flag_h = 1;
  }
  else{
    flag_h = 0;
  }

  time_based_interpolator.add_move(GLOBAL, 20.0, 0, 0, 0, 0, 0, 0);
}


void draw_radial() {

  radial_length = drawing_area / 2;

  for (int i = 0; i < radial_lines; i++) {
    time_based_interpolator.add_move(GLOBAL, 20, 0, 2 * PI * i / radial_lines, 4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 50, 0, 2 * PI * i / radial_lines, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, drawing_velocity, radial_length, 2 * PI * i / radial_lines + deviation * PI, -4, 0, 0, 0);
    time_based_interpolator.add_move(GLOBAL, 80, radial_length, 2 * PI * i / radial_lines + deviation * PI, 4, 0, 0, 0);
  }

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

  analog_a1.set_floor(4, 25);
  analog_a1.set_ceiling(25, 1020);

  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);
  analog_a2.map(&wave_gen_z.frequency);

  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(3, 1020);

  analog_a4.set_floor(0, 25);
  analog_a4.set_ceiling(4, 1020);

  line_count = analog_a1.read();
  spacing= drawing_area / line_count;
  line_length = spacing * (line_count - 1);

  xy_wave_scaler_a = analog_a3.read();
  xy_wave_scaler_f = analog_a4.read();

  wave_gen_x.frequency = basic_wave_x_f * xy_wave_scaler_f;
  wave_gen_x.amplitude = basic_wave_x_a * xy_wave_scaler_a;
  wave_gen_y.frequency = basic_wave_y_f * xy_wave_scaler_f;
  wave_gen_y.amplitude = basic_wave_y_a * xy_wave_scaler_a;

  polar_kinematics.disable();

  // ====== ADDED: cache live values for reporting ======
  live_a1 = line_count;
  live_a2 = analog_a2.read();
  live_a3 = xy_wave_scaler_a;
  live_a4 = xy_wave_scaler_f;
}


//=========================circle==========================
void draw_circle_setup(){

  position_gen_r.output.map(&polar_kinematics.input_radius);
  position_gen_r.begin();

  wave_gen_r.setNoInput();
  wave_gen_r.frequency = 0;
  wave_gen_r.amplitude = 0.6;
  wave_gen_r.begin();

  wave_gen_s.setNoInput();
  wave_gen_s.frequency = 0.1;
  wave_gen_s.amplitude = 0.2;
  wave_gen_s.begin();

  velocity_gen_r.begin();
  velocity_gen_r.speed_units_per_sec = 0.15;

  velocity_gen_a.begin();
  velocity_gen_a.speed_units_per_sec = 0.8;

}


void draw_circle_loop(){
  polar_kinematics.enable();

  wave_gen_r.output.map(&polar_kinematics.input_radius);
  wave_gen_s.output.map(&polar_kinematics.input_radius);
  velocity_gen_r.output.map(&polar_kinematics.input_radius);
  velocity_gen_a.output.map(&polar_kinematics.input_angle);

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

  // ====== cache live values for reporting ======
  live_a1 = analog_a1.read();
  live_a2 = analog_a2.read();
  live_a3 = analog_a3.read();
  live_a4 = analog_a4.read();
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

  analog_a1.set_floor(4, 25);
  analog_a1.set_ceiling(24, 1020);
  square_count = analog_a1.read();

  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);
  analog_a2.map(&wave_gen_z.frequency);

  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(3, 1020);

  analog_a4.set_floor(0, 25);
  analog_a4.set_ceiling(4, 1020);

  xy_wave_scaler_a = analog_a3.read();
  xy_wave_scaler_f = analog_a4.read();

  wave_gen_x.frequency = basic_wave_x_f * xy_wave_scaler_f;
  wave_gen_x.amplitude = basic_wave_x_a * xy_wave_scaler_a;
  wave_gen_y.frequency = basic_wave_y_f * xy_wave_scaler_f;
  wave_gen_y.amplitude = basic_wave_y_a * xy_wave_scaler_a;

  polar_kinematics.disable();

  // ====== cache live values for reporting ======
  live_a1 = square_count;
  live_a2 = analog_a2.read();
  live_a3 = xy_wave_scaler_a;
  live_a4 = xy_wave_scaler_f;
}


//=====================radial=============================
void draw_radial_setup(){

}

void draw_radial_loop(){
  time_based_interpolator.output_x.map(&polar_kinematics.input_radius);
  time_based_interpolator.output_y.map(&polar_kinematics.input_angle);

  analog_a1.set_floor(3, 25);
  analog_a1.set_ceiling(25, 1020);
  radial_lines = analog_a1.read();

  analog_a2.set_floor(0.0, 25);
  analog_a2.set_ceiling(0, 1020);
  analog_a2.map(&wave_gen_z.frequency);

  analog_a3.set_floor(0, 25);
  analog_a3.set_ceiling(1.5, 1020);
  deviation = analog_a3.read();

  analog_a4.set_floor(10, 25);
  analog_a4.set_ceiling(150, 1020);
  drawing_area = analog_a4.read();

  // ====== cache live values for reporting ======
  live_a1 = radial_lines;
  live_a2 = analog_a2.read();
  live_a3 = deviation;
  live_a4 = drawing_area;
}


//====================button functions=================
void button_on_press(){

Serial.println("{\"btn2\":\"press\"}");   // ====== report press ======

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

Serial.println("{\"btn2\":\"release\"}");   // ====== ADDED: report release ======

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
void set_drawing_velocity(float32_t velocity){
  drawing_velocity = velocity;
}

void set_basic_wave(float32_t frequency_x, float32_t amplitude_x, float32_t frequency_y, float32_t amplitude_y){

basic_wave_x_f = frequency_x;
basic_wave_x_a = amplitude_x;
basic_wave_y_f = frequency_y;
basic_wave_y_a = amplitude_y;
}

void set_basic_spacing(float32_t spacing){
  velocity_gen_r.speed_units_per_sec = spacing;
}

void set_spacing_variation(float32_t frequency, float32_t amplitude){
  wave_gen_s.frequency = frequency;
  wave_gen_s.amplitude = amplitude;
}

void set_pattern_mode(float32_t mode){
  pattern_mode = mode;
}

void set_drawing_area(float32_t area){
 drawing_area = area;
}

void hello_serial(){
  Serial.print("hello!");
}


void report_overhead(){

}


// ====== report live board state to the preview interface ======
void report_state(){

  Serial.print("{\"a\":[");
  Serial.print(live_a1, 2); Serial.print(",");
  Serial.print(live_a2, 2); Serial.print(",");
  Serial.print(live_a3, 2); Serial.print(",");
  Serial.print(live_a4, 2);
  Serial.print("],\"mode\":");
  Serial.print(pattern_mode);
  Serial.print(",\"motor\":");
  Serial.print(button_d1.read_raw() ? 1 : 0);
  Serial.println("}");
}
