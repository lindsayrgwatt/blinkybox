#include <avr/io.h>
#include <avr/interrupt.h>
#include "LPD8806.h"
#include "SPI.h"
#include "Encoder.h"

// Controls a toy box of blinking lights

/*****************************************************************************/

// Number of RGB LEDs in strand:
int nLEDs = 32;

// PHYSICAL VARIABLES
// Output pins for LED strand
int dataPin  = 2;
int clockPin = 3;

// Arcade buttons
const unsigned int redPin = 12;
const unsigned int yellowPin = 11;
const unsigned int bluePin = 10;
const unsigned int pinkPin = 9;
const unsigned int greenPin = 8;
const unsigned int whitePin = 7;

// Toggle switch for mode
const unsigned int togglePin = 6;
const unsigned int ledPin = 13;

 // Knob constants
const unsigned int encoderPinOne = 4;
const unsigned int encoderPinTwo = 5;

Encoder knob(encoderPinOne, encoderPinTwo);
long oldKnobState = 0;

// CODE VARIABLES
// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

volatile int color = 0;
volatile int chaseState = 0;
volatile int fadeState = 0;
volatile boolean globalState = false;

int reading;
volatile boolean animated = false; // Static vs. animated lights
volatile int animatedWaitTime = 50;
int delayBoost = 40;

int minAnimatedWaitTime = 10;
int maxAnimatedWaitTime = 100;

void setup() {
  Serial.begin(9600);
  Serial.println("Began setup");
  
  // Interrupts
  pinMode(redPin, INPUT_PULLUP);
  attachInterrupt(redPin, interruptRed, FALLING);
  pinMode(yellowPin, INPUT_PULLUP);
  attachInterrupt(yellowPin, interruptYellow, FALLING);
  pinMode(bluePin, INPUT_PULLUP);
  attachInterrupt(bluePin, interruptBlue, FALLING);
  pinMode(pinkPin, INPUT_PULLUP);
  attachInterrupt(pinkPin, interruptPink, FALLING);
  pinMode(greenPin, INPUT_PULLUP);
  attachInterrupt(greenPin, interruptGreen, FALLING);
  pinMode(whitePin, INPUT_PULLUP);
  attachInterrupt(whitePin, interruptWhite, FALLING);

  pinMode(togglePin, INPUT);
  attachInterrupt(togglePin, interruptToggle, CHANGE);
  
  pinMode(ledPin, OUTPUT);
  
  reading = digitalRead(togglePin);
  if (reading == HIGH) {
    animated = true;
    Serial.println("Start in animated model");
  } else {
    animated = false;
    Serial.println("Start in static mode");
  }
  digitalWrite(ledPin, reading);
  
  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
}

// Decider function. Can't be in main loop or long functions stall interrupts
void decider(boolean animated, int color) {
  if (animated == false) {
    switch(color) {
      case 1: // Red button
        fadeLights(animatedWaitTime * delayBoost, 127, 0, 0);
        break;
      case 2: // Yellow button
        fadeLights(animatedWaitTime * delayBoost, 127, 127, 0);
        break;
      case 3: // Blue button
        fadeLights(animatedWaitTime * delayBoost, 0, 0, 127);
        break;
      case 4: // Pink button
        fadeLights(animatedWaitTime * delayBoost, 127, 0, 127);
        break;
      case 5: // Green button
        fadeLights(animatedWaitTime * delayBoost, 0, 127, 0);
        break;
      default: // Also triggered by White button
        fadeLights(animatedWaitTime * delayBoost, 127, 127, 127);
    }
  } else {
    switch(color) {
      case 1: // Red button
        theaterChaseRainbow(animatedWaitTime);
        break;
      case 2: // Yellow button
        rainbowCycle(animatedWaitTime);
        break;
      case 3: // Blue button
        colorWipe(animatedWaitTime);
        break;
      case 4: // Pink button
        theaterChase(strip.Color(127, 127, 127), animatedWaitTime);
        break;
      case 5: // Green button
        raindropLights(animatedWaitTime);
        break;
      default: // Also triggered by White button
        twinkleLights(animatedWaitTime, 127, 127, 127);
    }
  }
}

void updateSpeed() {
  long delta = 0;
  long knobState = knob.read();
  
  if (animatedWaitTime >= maxAnimatedWaitTime && knobState >= oldKnobState) {
    knobState = oldKnobState;
  }
  
  if (animatedWaitTime <= minAnimatedWaitTime && knobState <= oldKnobState) {
    knobState = oldKnobState;
  }
  
  if (knobState != oldKnobState) {
    Serial.print("knobState: ");
    Serial.println(knobState);
    Serial.print("oldKnobState: ");
    Serial.println(oldKnobState);
    Serial.print("old animatedWaitTime: ");
    Serial.println(animatedWaitTime);
    delta = knobState - oldKnobState;
    Serial.print("delta: ");
    Serial.println(delta);
    Serial.println(animatedWaitTime - delta);
    animatedWaitTime = animatedWaitTime + delta;
    oldKnobState = knobState;
    
    if (animatedWaitTime > maxAnimatedWaitTime) {
      animatedWaitTime = maxAnimatedWaitTime;
    }
    if (animatedWaitTime < minAnimatedWaitTime) {
      animatedWaitTime = minAnimatedWaitTime;
    }
    Serial.print("new animatedWaitTime: ");
    Serial.println(animatedWaitTime);
    
  }
}

void loop() {
  globalState = false;
  
  //long knobState = knob.read();
  //Serial.println(knobState);
  updateSpeed();
  
  decider(animated, color);
}

// Interrupts
void interruptRed() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 1;
    Serial.println("Red interrupt"); 
  }
  last_interrupt_time = interrupt_time;
}

void interruptYellow() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 2;
    Serial.println("Yellow interrupt"); 
  }
  last_interrupt_time = interrupt_time;
}

void interruptBlue() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 3;
    Serial.println("Blue interrupt"); 
  }
  last_interrupt_time = interrupt_time;
}

void interruptPink() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 4;
    Serial.println("Pink interrupt");
  }
  last_interrupt_time = interrupt_time;
}

void interruptGreen() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 5;
    Serial.println("Green interrupt");
  }
  last_interrupt_time = interrupt_time;
}

void interruptWhite() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    globalState = true;
    color = 0;
    Serial.println("White interrupt");
  }
  last_interrupt_time = interrupt_time;
}

void interruptToggle() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    Serial.println("Change");
    reading = digitalRead(togglePin);
    if (reading == HIGH) {
      animated = true;
      digitalWrite(ledPin, reading);
      Serial.println("High");
    }
    if (reading == LOW) {
      animated = false;
      digitalWrite(ledPin, reading);
      Serial.println("Low");
    }
  }
  last_interrupt_time = interrupt_time;
}

// Static mode methods
void clearStrip() {
    int j;
    for (j=0; j<strip.numPixels(); j++) {
       strip.setPixelColor(j, 0, 0, 0); 
    }
    strip.show();
}

void setIfPresent(int idx, int r, int g, int b) {
   // Don't write to non-existent pixels
    if (idx >= 0 && idx < strip.numPixels()) 
        strip.setPixelColor(idx, r, g, b);
}

void fadeLights(int wait, int r, int g, int b) {
    const int transitions = 15;
    int j;
   
    int brightness = 0;
    if (fadeState < transitions) {
       brightness = fadeState;
    } else {
       brightness = transitions - (fadeState-transitions);   
    }
    float percentage = float(brightness)/float(transitions);
    for (j=0; j<strip.numPixels(); j++) {
      strip.setPixelColor(j, r * percentage, g * percentage, b *percentage);
      updateSpeed();
      if (globalState == true) {
         globalState = false;
       break;
      }
    }
    strip.show();
    delay(wait/transitions);
    fadeState = fadeState + 1;
    if (fadeState == 30)
      fadeState = 0;
}

// Animated mode methods
//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 384; j++) {     // cycle all 384 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 384));    //turn every third pixel on
          updateSpeed();
          if (globalState == true){ goto stateUpdate;}
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
          updateSpeed();
          if (globalState == true){goto stateUpdate;}
        }
    }
  }
  // Use a goto label here as two nested loops
  stateUpdate:
  globalState = false;
}

// Cycle through all the colors of the rainbow
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
      updateSpeed();
      if (globalState == true){goto stateUpdate;}
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
  // Use a goto label here as two nested loops
  stateUpdate:
  globalState = false;
}

// Fill the dots progressively along the strip.
void colorWipe(uint8_t wait) {
  for (int h=0; h < 7; h++) {
    uint32_t c;
    switch(h) {
      case 0:
        c = strip.Color(127, 127, 127); // White
        break;
      case 1:
        c = strip.Color(127, 0, 0); // Red
        break;
      case 2:
        c = strip.Color(127, 127, 0); // Yellow
        break;
      case 3:
        c = strip.Color(0, 127, 0); // Green
        break;
      case 4:
        c = strip.Color(0, 127, 127); // Cyan
        break;
      case 5:
        c = strip.Color(0, 0, 127); // Blue
        break;
      case 6:
        c = strip.Color(127, 0, 127); // Violet
        break;
    }
  
    int i;

    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      updateSpeed();
      if (globalState == true) {goto stateUpdate;}
      delay(wait);
    }
  }
  // Use a goto label here as two nested loops
  stateUpdate:
  globalState = false;
}

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
        updateSpeed();
        if (globalState == true) {goto stateUpdate;}
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
        updateSpeed();
        if (globalState == true) {goto stateUpdate;}
    }
    }
  }
  stateUpdate:
  globalState = false;
}

void raindropLights(int wait) {
  int r;
  int g;
  int b;
  
  for (int h=0; h < 7; h++) {
    switch(h) {
      case 0: // White
        r = 127;
        g = 127;
        b = 127;
        break;
      case 1: // Red
        r = 127;
        g = 0;
        b = 0;
        break;
      case 2: // Yellow
        r = 127;
        g = 127;
        b = 127;
        break;
      case 3: // Green
        r = 0;
        g = 127;
        b = 0;
        break;
      case 4: // Cyan
        r = 0;
        g = 127;
        b = 127;
        break;
      case 5: // Blue
        r = 0;
        g = 0;
        b = 127;
        break;
      case 6: // Violet
        r = 127;
        g = 0;
        b = 127;
        break;
    }
  
    clearStrip();
    int i = chaseState;

    setIfPresent(i-6, 0, 0, 0); 
    setIfPresent(i-5, r/32, g/32, b/32); 
    setIfPresent(i-4, r/16, g/16, b/16); 
    setIfPresent(i-3, r/8, g/8, b/8); 
    setIfPresent(i-2, r/4, g/4, b/4); 
    setIfPresent(i-1, r/2, g/2, b/2);
    setIfPresent(i, r, g, b );
    strip.show();           
    delay(wait*50/nLEDs);

    chaseState = chaseState + 1;
    if (chaseState == nLEDs)
      chaseState = 0;
    updateSpeed();
    if (globalState == true) {goto stateUpdate;}
  }
  stateUpdate:
  globalState = false;
}

void twinkleLights(int wait, int r, int g, int b) {
    clearStrip();
    int i;
    int j;
    int pixel;
    const int transitions = 15; 
    const int numTwinkles = strip.numPixels();
    for (j=0; j<numTwinkles; j++) {
       pixel = random(0, strip.numPixels());
       
       // Fade in
       for (i=0; i<transitions; i++) {
          float percentage = float(i)/float(transitions);
          strip.setPixelColor(pixel, r * percentage, g * percentage, b*percentage); 
          strip.show();
          updateSpeed();
          if (globalState == true) {goto stateUpdate;}
          delay(wait*40/numTwinkles/transitions);
       }
       // Fade out
       for (i=transitions; i>=0; i--) {
          float percentage = float(i)/float(transitions);
          strip.setPixelColor(pixel, r *percentage, g *percentage, b*percentage); 
          strip.show();
          updateSpeed();
          if (globalState == true) {goto stateUpdate;}
          delay(wait*40/numTwinkles/transitions);
       }
    }
    stateUpdate:
    globalState = false;
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
