#include "pins.h"

#define _LEN(x,y) sizeof(x) / sizeof(y)

#define FADESPEED 1     // make this higher to slow down

const int FANPORTS[4] = {FANPIN1, FANPIN2, FANPIN3, FANPIN4};
const int LOCKBANKS[2] = {LOCKSETPIN1, LOCKSETPIN2};
const int LOCKSWBANKS[2] = {LOCKSETSWPIN1, LOCKSETSWPIN2};

// Bank of LED Strips.
const int LEDBANKS[4][3] = {
   {REDPIN1, GREENPIN1, BLUEPIN1},
   {REDPIN2, GREENPIN2, BLUEPIN2},
   {REDPIN3, GREENPIN3, BLUEPIN3},
   {REDPIN4, GREENPIN4, BLUEPIN4}
};

int lockSwState[2] = {0,0};
int *currentFanSpeed = (int *)malloc(sizeof(FANPORTS));
int reqSw = 1;

void setup() {
  Serial.begin(9600);
  
  int i, x;

  // Set LED pin outputs.  
  for(i = 0; i < (_LEN(LEDBANKS, int)); i++) {
    for(x = 0; x < 3; x++) {
      pinMode(LEDBANKS[i][x], OUTPUT);
    }
  }

  for(i = 0; i < (_LEN(LOCKBANKS, int)); i++) {
    pinMode(LOCKBANKS[i], OUTPUT);
  }

  for(i = 0; i < (_LEN(LOCKSWBANKS, int)); i++) {
    pinMode(LOCKSWBANKS[i], INPUT);
  }

  for(i = 0; i < (_LEN(FANPORTS, int)); i++) {
    pinMode(FANPORTS[i], OUTPUT);
  }

  pinMode(RELAYPIN, OUTPUT);

  // Null out the current fan speeds.
  memset(currentFanSpeed, 0, sizeof(FANPORTS));
}
 
void setLED(int bank, char *rgb) {
  int red, green, blue;
  long color;

  color = strtol(rgb, NULL, 16);
  
  red = ((color >> 16) & 0xFF);
  green = ((color >> 8) & 0xFF);
  blue = ((color) & 0xFF);
  
  analogWrite(LEDBANKS[bank][0], red);
  analogWrite(LEDBANKS[bank][1], green);
  analogWrite(LEDBANKS[bank][2], blue); 
}

void loop() {
  // Get switch states.
  lockSwState = {digitalRead(LOCKSETSWPIN1), digitalRead(LOCKSETSWPIN2)};
 
  if (Serial.available() > 0) {
    int c = Serial.read();
    int r;
    
    switch (c) {
      case 'c': // Set LED Bank Color: c[bank][rrggbb]  eg. c0ffffff
        char colorBuff[8];
        char bankBuff[2];
        int bank;
        
        r = Serial.readBytes(colorBuff, 7);
        colorBuff[7] = 0;
        
        bankBuff = {colorBuff[0], 0};
        bank = strtol(bankBuff, NULL, 10);  
        
        setLED(bank, &colorBuff[1]);
        
        break;
      case 'g': // Get info
        int x;
        for(x = 0; x < _LEN(FANPORTS, int); x++) {
          Serial.print("f");
          Serial.print(x, DEC);
          Serial.println(currentFanSpeed[x], DEC);
        }
        break;
      case 'f': // Fan Control: f[bank][speed in hex]  eg. f1ff
        char opts[4];
        int fanSpeed;
        int fbank;
      
        r = Serial.readBytes(opts, 3);
        opts[3] = 0;
      
        fbank = opts[0] - 48;
        fanSpeed = strtol(&opts[1], NULL, 16);
        
        Serial.print("Setting fan ");
        Serial.print(fbank, DEC);
        Serial.print(" with speed ");
        Serial.println(fanSpeed, HEX);
      
        currentFanSpeed[fbank] = fanSpeed;
      
        analogWrite(FANPORTS[fbank], fanSpeed);
      
        break;
      case 'l': // Lock Bank: l[bank]  eg. l1
        char lbank;
        Serial.readBytes(&lbank, 1);        
        
        if (!lockSwState[lbank-48] || !reqSw)
          digitalWrite(LOCKBANKS[lbank-48], HIGH);
        break;
      case 'u': // Unlock Bank: u[bank]  eg. u0
        char ubank;
        Serial.readBytes(&ubank, 1);
        Serial.println(ubank);23
        digitalWrite(LOCKBANKS[ubank-48], LOW);
        break;
      case 'r': // "press" switch connected to relay.
        digitalWrite(RELAYPIN, HIGH);
        delay(1000);
        digitalWrite(RELAYPIN, LOW);
        break;
      case 'x': // Toggle the switch press lock requirement.
        reqSw = !reqSw;
        break;
    }
  }
}

 
