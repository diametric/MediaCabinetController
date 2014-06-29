#include "pins.h"

#define _LEN(x,y) sizeof(x) / sizeof(y)

#define RELAY_DELAY 1000  // Delay between toggles of the relay (to mimic user input on the HDMI switch)

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

int *lockSwState = (int *)malloc(sizeof(LOCKSWBANKS));
long *ledState = (long *)malloc(_LEN(LEDBANKS, int) * sizeof(long));
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
  memset(ledState, 0, sizeof(_LEN(LEDBANKS, int) * sizeof(long)));
  memset(lockSwState, 0, sizeof(LOCKSWBANKS));
}
 
void setLED(int bank, long color) {
  int red, green, blue;
  
  red = ((color >> 16) & 0xFF);
  green = ((color >> 8) & 0xFF);
  blue = ((color) & 0xFF);
  
  analogWrite(LEDBANKS[bank][0], red);
  analogWrite(LEDBANKS[bank][1], green);
  analogWrite(LEDBANKS[bank][2], blue); 
}

void loop() {
  int i = 0;
  int swState;
  
  // Get switch states.
  for(i = 0; i < _LEN(LOCKSWBANKS, int); i++) {
    swState = digitalRead(LOCKSWBANKS[i]);
    
    if (swState != lockSwState[i]) {
      lockSwState[i] = swState;

      if (lockSwState[i] == HIGH) {
        Serial.println("Door opened.");
        setLED(0, 0xffffff);
      } else {
        Serial.println("Door closed.");
        setLED(0, ledState[0]);
      }
    }
  }
 
  if (Serial.available() > 0) {
    int c = Serial.read();
    int r;
    
    switch (c) {
      case 'c': // Set LED Bank Color: c[bank][rrggbb]  eg. c0ffffff
        char colorBuff[8];
        char bankBuff[2];
        int bank;
        long color;
        
        r = Serial.readBytes(colorBuff, 7);
        colorBuff[7] = 0;
        
        bankBuff = {colorBuff[0], 0};
        bank = strtol(bankBuff, NULL, 10);          
        color = strtol(&colorBuff[1], NULL, 16);
        
        ledState[bank] = color;        
        setLED(bank, color);
        
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
        Serial.println(ubank);
        digitalWrite(LOCKBANKS[ubank-48], LOW);
        break;
      case 'r': // "press" switch connected to relay.
        digitalWrite(RELAYPIN, HIGH);
        delay(RELAY_DELAY);
        digitalWrite(RELAYPIN, LOW);
        break;
      case 'x': // Toggle the switch press lock requirement.
        reqSw = !reqSw;
        break;
    }
  }
}

 
