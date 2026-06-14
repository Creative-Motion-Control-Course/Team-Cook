/*
This sketch is for the stepdance basic module of the paper_embroidery peoject
input includes: 
1. pantograph (two encoders)
2. button_1: to identify recording strat/stop
3. button_2: to enable playback + send signal to the driver_module
4. slider_1: control the scaling filter when replay
5. slider_2: control the parsing length (path_length generator)


*/

#define module_basic 
#include "stepdance.hpp"

// -- Define Output Ports --
// We'll run everything out over a single output port

OutputPort output_a;  // Basic Module output port

// -- Define Motion Channels --
Channel channel_x;
Channel channel_y;
Channel channel_z; 

// -- Define Kinematics --
KinematicsPolarToCartesianOffset polar_kinematics_offset;

// -- Define Encoders --
Encoder encoder_theta;   //bottom, enc_1
Encoder encoder_radius;  //top, enc_2

// -- Button Input --
Button record_button; //D1
Button play_button; //D2

// -- Analog Input --
AnalogInput scaling_slider_a1;
AnalogInput parsing_slider_a2; 

// -- Scaling --
ScalingFilter2D scaling_filter;



// -- Record & Playback --
FourTrackRecorder recorder;
FourTrackPlayer player;

// -- Position Gen --
PositionGenerator position_gen_x;
PositionGenerator position_gen_y;
PositionGenerator position_gen;

// -- TBI---
TimeBasedInterpolator TBI;

// --other---
PathLengthGenerator2D path_length_gen;
float64_t path_length_at_start = 0;

ControlParameter punch_interval;   
float64_t next_punch_at = 0;
bool punch_armed = false;


void setup() {
  // -- Configure and start the output ports --
  output_a.begin(OUTPUT_A);

  // -- Configure and start the channels --
  channel_x.begin(&output_a, SIGNAL_X);
  channel_x.set_ratio(0.01, 1); //basic resolution of 1 output step = 0.01mm
  
  channel_y.begin(&output_a, SIGNAL_Y);
  channel_y.set_ratio(0.01, 1);

  channel_z.begin(&output_a, SIGNAL_Z);
  channel_z.set_ratio(0.01, 1);

  // -- Configure and start the encoders --
  encoder_theta.begin(ENCODER_2);  // BOTTOM ENCODER
  encoder_theta.set_ratio(TWO_PI, 4096); //E6B2-CWZ3E resolution up to 1024ppr
  //encoder_theta.invert();
  encoder_theta.output.map(&polar_kinematics_offset.input_angle);


  encoder_radius.begin(ENCODER_1);  // TOP ENCODER
  encoder_radius.set_ratio(95, 4096); ////E6B2-CWZ3E resolution up to 1024ppr(x4), about 98mm travle length for one round
  encoder_radius.invert();
  encoder_radius.output.map(&polar_kinematics_offset.input_radius);
  

  // -- Configure and start the kinematics modules --
  //polar_kinematics_offset.output_x.map(&scaling_filter.input_1);
  //polar_kinematics_offset.output_y.map(&scaling_filter.input_2); 

  // polar_kinematics_offset.output_x.map(&channel_y.input_target_position);
  // polar_kinematics_offset.output_y.map(&channel_x.input_target_position);

  polar_kinematics_offset.begin(19); //out the offset d value into it (r + h/2)

  // // -- Scaling Filter --
  scaling_filter.begin(); //defaults to incremental mode
  //swap x, y when output
  scaling_filter.output_1.map(&channel_y.input_target_position); 
  scaling_filter.output_2.map(&channel_x.input_target_position);
  scaling_filter.ratio = 1.0;

  scaling_slider_a1.begin(IO_A1);
  scaling_slider_a1.set_floor(0.1);
  scaling_slider_a1.set_ceiling(1);//can only scaling down
  scaling_slider_a1.set_deadband(1, 509, 4); //what are the numbers mean?
  // scaling_slider_a1.map(&scaling_filter.ratio);

  parsing_slider_a2.begin(IO_A2);
  parsing_slider_a2.set_floor(2);             //minimal gap 2mm
  parsing_slider_a2.set_ceiling(25);          //maximum gap 25mm
  parsing_slider_a2.map(&punch_interval);
  // punch_interval = 5.0;

  // -- buttons --
  record_button.begin(IO_D1, INPUT_PULLDOWN);
  record_button.set_mode(BUTTON_MODE_TOGGLE);
  record_button.set_callback_on_press(&start_recording);
  record_button.set_callback_on_release(&stop_recording);

  play_button.begin(IO_D2, INPUT_PULLDOWN);
  play_button.set_mode(BUTTON_MODE_STANDARD);
  play_button.set_callback_on_press(&playback_control);

  // play_button.begin(IO_D2, INPUT_PULLDOWN);
  // record_button.set_mode(BUTTON_MODE_TOGGLE);
  // record_button.set_callback_on_press(&playback_control);
  // record_button.set_callback_on_release(&playback_control);

  recorder.input_1.map(&polar_kinematics_offset.output_x);
  recorder.input_2.map(&polar_kinematics_offset.output_y);
  recorder.begin();

  player.output_1.map(&scaling_filter.input_1);
  player.output_2.map(&scaling_filter.input_2);
  player.begin();

  //disable the output channel, so the plotter won't move with the pantograph, only enable it when playback
  channel_x.disable();
  channel_y.disable();

  // //TBI set up
  TBI.output_z.map(&channel_z.input_target_position);

  TBI.begin();

  //  --path length gen--
  path_length_gen.input_1.map(&scaling_filter.output_1);
  path_length_gen.input_2.map(&scaling_filter.output_2);
  path_length_gen.set_ratio(1.0);  // output.distance = 1.0 * input.distance mm
  path_length_gen.begin();

  //position generator
  // position_gen_x.output.map(&channel_x.input_target_position);
  // position_gen_y.output.map(&channel_y.input_target_position);

  // position_gen_x.begin();
  // position_gen_y.begin();

  dance_start();
}


LoopDelay overhead_delay;

void loop() {
  overhead_delay.periodic_call(&report_overhead, 500);

  if (punch_armed) {
    float64_t current_path_length = path_length_gen.output.read_absolute();
    if (current_path_length >= next_punch_at) {
      player.pause();
      punch_action();
      delay(2000);
      player.resume();
      next_punch_at += punch_interval;   //set the next punch hole
    }
  }

  dance_loop(); 


  if(player.playback_active){
  channel_x.enable();
  channel_y.enable();
  punch_armed = true;

    //disable the encoder to avoid motion conflict by accidental touch
  encoder_radius.output.disable();
  encoder_theta.output.disable();

  }else{
      //disable output channel again
  channel_x.disable();
  channel_y.disable();
  //enable encoder to track position
  encoder_radius.output.enable();
  encoder_theta.output.enable();

  punch_armed = false;

  }

}


void start_recording(){
  recorder.start("recording_name");
  //path_length_at_start = path_length_gen.output.read_absolute();
  Serial.println("STARTED RECORDING");
  Serial.print(" | punch every ");
  Serial.print(punch_interval, 1);
  Serial.println(" mm");
}
 
void stop_recording(){
  recorder.stop();
  Serial.println("STOPPED RECORDING");

  //float64_t total_length = path_length_gen.output.read_absolute() - path_length_at_start;
  // Serial.print("STOPPED RECORDING | total path length = ");
  // Serial.print(total_length, 3);
  // Serial.println(" mm");

}


void playback_control(){
  if(player.playback_active){
    stop_playback();
  }else{
    start_playback();
  }
}


void start_playback(){
  //path_length_at_start = path_length_gen.output.read_absolute();
  next_punch_at = punch_interval;

  path_length_gen.output.reset(0);

  player.start("recording_name");

  Serial.println("STARTED PLAYING");

  int duration_s = player.max_num_playback_samples / (CORE_FRAME_FREQ_HZ);

  Serial.print("DURATION: ");
  Serial.print(static_cast<int>(duration_s / 60));
  Serial.print("m:");
  Serial.print(duration_s % 60);
  Serial.println("s");


  //maybe put this into driver module
  // position_gen_x.go(0, ABSOLUTE, 100);
  // position_gen_y.go(0, ABSOLUTE, 100);
  

}
 
void stop_playback(){
  player.stop();
}

void punch_action(){
  TBI.add_timed_move(ABSOLUTE, 1, 0, 0, -8, 0, 0, 0);
  //TBI.add_timed_move(ABSOLUTE, 1, 0, 0, -8, 0, 0, 0);
  TBI.add_timed_move(ABSOLUTE, 0.08, 0, 0, 4, 0, 0, 0);

  
  Serial.print("PUNCH at ");
  Serial.print(next_punch_at, 2);
  Serial.println(" mm");
}


void report_overhead(){
  //Serial.println(scaling_slider_a1.last_value_raw);
  // Serial.print("scaling_slider read: ");
  Serial.println(scaling_slider_a1.read());
  Serial.println(parsing_slider_a2.read());
  // Serial.print("cha x: ");
  // Serial.println(channel_x.input_target_position.read(ABSOLUTE));
  // Serial.print("cha y: ");
  // Serial.println(channel_y.input_target_position.read(ABSOLUTE));

  // Serial.print("x: ");
  // Serial.println(polar_kinematics_offset.output_x.read(ABSOLUTE));
  // Serial.print("y: ");
  // Serial.println(polar_kinematics_offset.output_y.read(ABSOLUTE));
  

  // Serial.print("enc radius: ");
  // Serial.println(encoder_radius.output.read(ABSOLUTE));
  // Serial.print("enc angle: ");
  // Serial.println(encoder_theta.output.read(ABSOLUTE));

}


