/*
 * Copyright (c) 2018, circuits4you.com
 * All rights reserved.
 * 
 * ESP32 DAC - Digital To Analog Conversion Example
 */

#define DAC1 25
#define DAC2 26

void setup() {
  Serial.begin(115200);
  
}

void loop() { // Generate a Sine wave
  static int v; //255= 3.3V 128=1.65V
  static int dir;
  dacWrite(DAC1, v);
  dacWrite(DAC2, 255-v);
  if(dir){
    v++;
    if(v>255){
      v = 255;
      dir = 0;
    }
  }else{
    v--;
    if(v<0){
      v = 0;
      dir = 1;
    }
  }
  delay(20);
}
