// uTubo MK_I -> "Ao alcance de Todos", Serviço Educativo da Casa da Musica
// Tiago Angelo (March 2013)
//STATUS: adding mapping design 

#include <MozziGuts.h>        // mozzi lib
#include <Oscil.h>           // Oscil class to generate inpupt pulse
#include <tables/PINKNOISE8192_int8.h> // pink pinkNoise wavetable
#include <Ead.h>          // Exponential attack-decay class
#include <EventDelay.h>  // Event delay class
#include <AudioDelay.h> // Audio delay class
#include <fixedMath.h>// Fixed math 
//#include <mozzi_midi.h>// Midi utility (mtof conversion)
 
#define CONTROL_RATE 256 // or some other power of 2 (min=64) (max for this sketch=128)
#define MAX_DELAY 1024 // maximum delay size 
#define MIN_DELAY 125 // minimum delay size 

#define RAMP 8 // envelope (attack = decay) in ms
#define METRO_MIN 80 // envelope duration in ms


#define ZERO 0 
#define SENSORVAL 1023
#define REST_THRESH 6 // resting threshold for flex sensors

#define BUTTON_PIN 7

// IMPULSE VARIABLES  
  Oscil<PINKNOISE8192_NUM_CELLS, AUDIO_RATE> pinkNoise(PINKNOISE8192_DATA); // pink noise oscillator
  Ead impulseEnvelope(CONTROL_RATE); // envelope 
  EventDelay <CONTROL_RATE> impulseDuration; // duration 
  
// DELAY VARIABLES 
  AudioDelay <MAX_DELAY> leftDelay; 
  Q0n31 leftDelaySize = 256;
  AudioDelay <MAX_DELAY> rightDelay; 
  Q0n31 rightDelaySize = 256; 

// SENSOR VARIABLES 
  Q0n31 memb, membrane;
  uchar leftFlex, rightFlex;
  uchar restLFlex, restRFlex; 
  uchar button; 
  uchar prev_button;
  boolean restTrigger; // resting state trigger  
  
//SYNTH VARIABLES 
  boolean metro = true;
  Q0n31 metroTime = 2000; 
//SIGNAL VARIABLES 
  char impulse = 0, impulseGain, leftSig = 0, rightSig = 0;
  
///////////////////////////////////////////////INIT////////////////////////////////////////////////////  
void setup() {
  pinMode(BUTTON_PIN, INPUT);
  
  startMozzi(CONTROL_RATE);
  
  pinkNoise.setFreq((float) 8);
  //pinkNoise.setFreq((float)AUDIO_RATE/PINKNOISE8192_NUM_CELLS); // set pinkNoise osc freq (curr = 2)
  impulseDuration.start(20); 

}

///////////////////////////////////////////////CONTROL/////////////////////////////////////////////////
void updateControl() {
  
  readSensors(); 
  mapping(); // map sensor to synth params 
  setImpulse(); // impulse source  
}

///////////////////////////////////////////////AUDIO///////////////////////////////////////////////////
int updateAudio() {
  // IMPULSE
  impulse = (pinkNoise.next()*impulseGain); 
  
  // DELAY BLOCK W/ FEEDBACK 
  leftSig = leftDelay.next(impulse + leftSig, leftDelaySize); 
  rightSig = rightDelay.next(impulse + rightSig, rightDelaySize);
  
  //OUTPUT 
  return leftSig+rightSig; 
}

//////////////////////////////////////////OTHER_FUNCTIONS//////////////////////////////////////////////
void readSensors() { //READ SENSOR DATA

  memb = analogRead(A2); 
  if(memb>0){ membrane = (SENSORVAL - memb)>>1; } // 1~512 (this should be half of MAX_DELAY)

  leftFlex = analogRead(A0)-200; 
  rightFlex = analogRead(A1)-200;

  prev_button = button; 
  button = digitalRead(BUTTON_PIN);
  //button = digitalRead
  if (button != prev_button) { restTrigger = true; }
  else { restTrigger = false; } 
  
  // SET RESTING STATE 
  if (restTrigger) {
     restLFlex = leftFlex; 
     restRFlex = rightFlex; 
     impulseDuration.start(ZERO); //restart metro
  }
}

void mapping() {   //SENSOR->SYNTH MAPPING
  
  // SET METRO 
  if(leftFlex < restLFlex + REST_THRESH && leftFlex > restLFlex - REST_THRESH 
      && rightFlex < restRFlex + REST_THRESH && rightFlex > restRFlex - REST_THRESH )
  {
    metro = false; 
  } else { metro = true; } 
  
  // SET METRO TIME 
  metroTime = (((leftFlex * rightFlex)*(membrane/128))>>7) + METRO_MIN;
  
  // SET DELAY TIME (wich corresponds to heard pitch)
  leftDelaySize = membrane + leftFlex + MIN_DELAY;
  rightDelaySize = membrane + rightFlex + MIN_DELAY;
}

void setImpulse() { // SOURCE IMPULSE
  
  pinkNoise.setPhase(analogRead(A6)<<3); // cheap way of avoiding a noticeable loop (0~8192 noise)
  
  //IMPULSE ENVELOPE 
  if(impulseDuration.ready() == true && metro == true) {
    impulseEnvelope.start(RAMP, RAMP); // impulse attack-decay values
    impulseDuration.start(metroTime); // impulse duration 
  }   
  impulseGain = impulseEnvelope.next();
}



///////////////////////////////////////////////LOOP////////////////////////////////////////////////////
void loop() {
  audioHook(); // fills the audio buffer
  
}
