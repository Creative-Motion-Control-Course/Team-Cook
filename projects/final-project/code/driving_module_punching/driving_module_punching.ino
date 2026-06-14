#define module_driver

#include "stepdance.hpp"

// --define input and output ports--
OutputPort output_a;  // Axidraw left motor
OutputPort output_b;  // Axidraw right motor
OutputPort output_c;  // Z axis, a servo driver for the AxiDraw

InputPort input_b;  // from basic module

// -- Define Motion Channels --
Channel channel_a;  //AxiDraw "A" axis --> left motor motion
Channel channel_b;  // AxiDraw "B" axis --> right motor motion
Channel channel_z;  // AxiDraw "Z" axis --> pen up/down

// -- Define Kinematics --
KinematicsCoreXY axidraw_kinematics;

 //

// -- define positioning encoders --
Encoder enc_1;
Encoder enc_2; 

// --define buttons--
Button button_enc;

TimeBasedInterpolator TBI;

VelocityGenerator velocity_gen;

void setup() {
  // put your setup code here, to run once:

  // --configure the input stream--
  input_b.begin(INPUT_B);  // for a machine controller the parameter for input.begin should correspond with the physical port letter (A-D). Inputs can still receive up to four different signals of motion streams on the machine controller, there's just more of them physically as well.

  input_b.output_x.set_ratio(0.01, 1);  //1 step is 0.01mm - this should probably match the mapping from your upstream output
  input_b.output_x.map(&axidraw_kinematics.input_x);
  //

  input_b.output_y.set_ratio(0.01, 1);  //1 step is 0.01mm
  input_b.output_y.map(&axidraw_kinematics.input_y);
  //

  input_b.output_z.set_ratio(0.01, 1);  //?
  input_b.output_z.map(&channel_z.input_target_position);

  // -- Configure and start the output ports --
  output_a.begin(OUTPUT_A);  // "OUTPUT_A" specifies the physical port on the PCB for the output.
  output_b.begin(OUTPUT_B);
  output_c.begin(OUTPUT_C);

  enable_drivers();

  // -- Configure and start the channels --
  channel_a.begin(&output_a, SIGNAL_E);  // Connects the channel to the "E" signal on "output_a".
  channel_a.set_ratio(25.4, 2032);  // Sets the input/output transmission ratio for the channel.
                                    // This provides a convenience of converting between input units and motor (micro)steps
                                    // For the axidraw, 25.4mm == 2874 steps
  channel_a.invert_output();        // CALL THIS TO INVERT THE MOTOR DIRECTION IF NEEDED
  //channel_a.enable_filtering();

  channel_b.begin(&output_b, SIGNAL_E);
  channel_b.set_ratio(25.4, 2032);
  channel_b.invert_output();
  //channel_a.enable_filtering();

  channel_z.begin(&output_c, SIGNAL_E);  //servo motor, so we use a long pulse width
  channel_z.set_ratio(1, 50);            //straight step pass-thru. If use 25kg motor, change ratio to (1,1)

  // --configure TBI--
  TBI.output_x.map(&axidraw_kinematics.input_x);
  TBI.output_y.map(&axidraw_kinematics.input_y);
  TBI.output_z.map(&channel_z.input_target_position);

  TBI.begin();

  // -- configure kinematics--
  axidraw_kinematics.begin();
  axidraw_kinematics.output_a.map(&channel_a.input_target_position);
  axidraw_kinematics.output_b.map(&channel_b.input_target_position);

////we don't need this anymore since the playback motion is relative anyway
 // axidraw_kinematics.input_y.reset_deep(-182);
 // axidraw_kinematics.input_x.reset_deep(28);

  //setting up the posiitoning encoders
  enc_1.begin(ENCODER_1); 
  enc_1.set_ratio(24, 2400); 
  enc_2.begin(ENCODER_2); 
  enc_2.set_ratio(24, 2400); 

  enc_1.output.map(&axidraw_kinematics.input_x);
  enc_2.output.map(&axidraw_kinematics.input_y);


  //setting up buttons for enabling/disabling enc
  button_enc.begin(IO_D2, INPUT_PULLDOWN);
  button_enc.set_mode(BUTTON_MODE_TOGGLE);

  dance_start();
}


LoopDelay overhead_delay;


void loop() {
  // put your main code here, to run repeatedly:
  overhead_delay.periodic_call(&report_overhead, 500);

if (button_enc.read_raw()) {
  enable_enc();
} else {
  disable_enc();
}

  dance_loop();

  //
}

void enable_enc(){
  enc_1.output.enable();
  enc_2.output.enable();
}

void disable_enc(){
  enc_1.output.disable();
  enc_2.output.disable();
}

void report_overhead() {

  // Serial.print("cha a: ");
  // Serial.println(channel_a.input_target_position.read(ABSOLUTE));
  // Serial.print("cha b: ");
  // Serial.println(channel_b.input_target_position.read(ABSOLUTE));

  // Serial.print("axidraw x: ");
  // Serial.println(axidraw_kinematics.input_x.read(ABSOLUTE));
  // Serial.print("axidraw y: ");
  // Serial.println(axidraw_kinematics.input_y.read(ABSOLUTE));

  // Serial.print("input x: ");
  // Serial.println(input_b.output_x.read(ABSOLUTE));
  // Serial.print("input y: ");
  // Serial.println(input_b.output_y.read(ABSOLUTE));

  Serial.print("enc 1: ");
  Serial.println(enc_1.output.read(ABSOLUTE));
  Serial.print("enc 2: ");
  Serial.println(enc_2.output.read(ABSOLUTE));
}