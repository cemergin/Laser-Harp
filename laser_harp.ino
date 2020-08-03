/*
 * Project Laser Harp
 * Components: SparkFun PID 13906
 * Author: Cem Ergin
 * Date: Nov 18th 2019
 */

// MIDI USB Dependencies
#include <pitchToFrequency.h>
#include <pitchToNote.h>
#include <frequencyToNote.h>
#include <MIDIUSB_Defs.h>
#include <MIDIUSB.h>

// DEBUG 
#define SHOULD_DEBUG 0

// NUMBER OF NOTES
#define NUM_NOTES 5

// VARIABLES FOR SELECTOR OUTPUT
#define SELECTOR_ONE 4
#define SELECTOR_TWO 5
#define SELECTOR_THREE 6

#define MIDI_NOTE_ON_EVENT  0x09
#define MIDI_NOTE_OFF_EVENT  0x08
#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0

// FIRST NOTE IN SERIES
#define MIDI_A3 57
#define MIDI_B3 59
#define MIDI_C4 60
#define MIDI_D4 62
#define MIDI_E4 64 
#define MIDI_F4 65
#define MIDI_G4 67
#define MIDI_A4 69
#define MIDI_B4 71
#define MIDI_C5 72

// Sensor Index to switch between sensors
int sensorIndex = 0;
bool noteOn[NUM_NOTES];
int noteUp[NUM_NOTES];
int noteDown[NUM_NOTES];
int threshold[NUM_NOTES];
int midiNotes[] = {MIDI_A3, MIDI_C4, MIDI_D4, MIDI_E4, MIDI_G4, MIDI_A4, MIDI_B3, MIDI_F4}; // A Minor Pentatonic Scale

// Photoresistor Stiff
int photoCellReading = 0;
int photoCellPin = A0;

void setup() {
  // Digital Pin Setup
  
  pinMode(SELECTOR_ONE, OUTPUT);
  pinMode(SELECTOR_TWO, OUTPUT);
  pinMode(SELECTOR_THREE, OUTPUT);

  // Start Serial Output
  Serial.begin(115200);
  Serial.println("Laser Harp v1.0");
  delay(3000);
  
  // Setup Sensors
  setupSensors();
  // Initialize Note On Off Arrays
  setupNotes();
}

void loop() {
  photoCellReading = analogRead(photoCellPin);
  noteUp[sensorIndex] = averageInput(noteUp[sensorIndex], photoCellReading);

  if(SHOULD_DEBUG) {
  Serial.println("Smooth: " + String(noteUp[sensorIndex]) + " Val: " + String(photoCellReading) + " Chan: " + String(sensorIndex));
  }
  
  if (noteUp[sensorIndex] < threshold[sensorIndex]) {
    if(!noteOn[sensorIndex]) {
      // Object got close, start note
      noteOn[sensorIndex] = true;
      // Send note on MIDI message
      noteOnMIDI(midiNotes[sensorIndex], 100);
      if(SHOULD_DEBUG) {
      Serial.println("Note On: " + midiNotes[sensorIndex]);
        }
      }    
  } else {
    if (noteOn[sensorIndex]) {
      // Object got away, end note
      noteOn[sensorIndex] = false;
      // Send note off MIDI message
      noteOffMIDI(midiNotes[sensorIndex], 0);
      if(SHOULD_DEBUG) {
      Serial.println("Note Off: " + midiNotes[sensorIndex]);
        }
      }
    }
 incrementSensorIndex();
}

void noteOffMIDI(int pitch, int velocity) {
   midiEventPacket_t noteOff = {MIDI_NOTE_OFF_EVENT, MIDI_NOTE_OFF, pitch, velocity};
   MidiUSB.sendMIDI(noteOff);
   MidiUSB.flush();  
}

void noteOnMIDI(int pitch, int velocity) {
   midiEventPacket_t noteOn = {MIDI_NOTE_ON_EVENT, MIDI_NOTE_ON, pitch, velocity};
   MidiUSB.sendMIDI(noteOn);
   MidiUSB.flush();  
}

// Sets up the noteOn, midiNote and lastNote arrays depending on NUM_NOTES
void setupNotes() { 
  if(SHOULD_DEBUG) {
    Serial.println("Initializing MIDI Notes");
  }
  for ( int i = 0; i < NUM_NOTES; i++) {
    noteOn[i] = false;
    if(SHOULD_DEBUG) {
    Serial.println("Sensor #" + String(i) + " Notes: " + String(midiNotes[i]));
    }  
  }
  return;  
}

void setupSensors() {
  zeroArray(noteUp);
  zeroArray(noteDown);
  
  sensorIndex = 0;
  selectSensor(sensorIndex);
  int reading = 0;

  // Calibrate w/ lights ON
  Serial.println("Calibration Step #1: Illuminate Sensors");
  countDown(10, 1000);
  Serial.println("Initiating");
  delay(1000);
  for(int i=0; i<NUM_NOTES;i++) {
    for(int i=0; i<100;i++) {
      reading = analogRead(photoCellPin);
      noteUp[sensorIndex] = averageInput(noteUp[sensorIndex],reading);
    }
  Serial.println("Sensor # " + String(sensorIndex) + " : " + "Note Up :" + String(noteUp[sensorIndex]));
  incrementSensorIndex();
  }

  // Calibrate w/ lights OFF
  Serial.println("Calibration Step #2: Cover Sensors");
  countDown(10, 1000);
  Serial.println("Initiating");
  delay(1000);
  for(int i=0; i<NUM_NOTES;i++) {
    for(int i=0; i<100;i++) {
      reading = analogRead(photoCellPin);
      noteDown[sensorIndex] = averageInput(noteUp[sensorIndex],reading);
    }
  Serial.println("Sensor # " + String(sensorIndex) + " : " + "Note Down :" + String(noteDown[sensorIndex]));
  threshold[sensorIndex] = ((noteUp[sensorIndex] +  noteDown[sensorIndex]) / 2);
  incrementSensorIndex();
 }
  return;  
}

// Selects sensor to read using SparkFun PID 13906
void selectSensor(int i) {
  int mod0 = i % 2;
  int mod1 = (i / 2) % 2;
  int mod2 = (i / 4) % 2;

  if (mod0 >= 1) {
    digitalWrite(SELECTOR_ONE, HIGH);
  } else {
    digitalWrite(SELECTOR_ONE, LOW);
  }

  if (mod1 >= 1) {
    digitalWrite(SELECTOR_TWO, HIGH);
  } else {
    digitalWrite(SELECTOR_TWO, LOW);
  }  
  if (mod2 >= 1) {
    digitalWrite(SELECTOR_THREE, HIGH);
  } else {
    digitalWrite(SELECTOR_THREE, LOW);
  } 
}

void incrementSensorIndex() {
  sensorIndex++;
  if ( sensorIndex >= NUM_NOTES ) {
  sensorIndex = 0;
  } 
  selectSensor(sensorIndex);  
}

void zeroArray(int ary[]) {
  for(int i=0; i<NUM_NOTES;i++) {
    ary[i] = 0;  
  }
}

int averageInput(int avg, int input) {
  return ((avg * 9) + input) / 10;
}

void countDown(int num, int mil) {
Serial.println("Counting down from " + String(num * mil / 1000) + "seconds");
for (int i=0; i<num;i++) {
int k = num - i;
Serial.println("Countdown: " + String(k));
delay(mil);
}
}
