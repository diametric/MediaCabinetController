#include "pins.h"

#define _LEN(x,y) sizeof(x) / sizeof(y)

// Configuration

#define DEBOUNCE 20
#define RELAY_DELAY 1000  // Delay between toggles of the relay (to mimic user input on the HDMI switch)
#define DOOROPEN_COLOR 0xFFFFFF

// Constants

const int FANPORTS[4] = {FANPIN1, FANPIN2, FANPIN3, FANPIN4};
const int LOCKBANKS[2] = {LOCKSETPIN1, LOCKSETPIN2};
const int LOCKSWBANKS[2] = {LOCKSETSWPIN1, LOCKSETSWPIN2};
const int LEDBANKS[4][3] = {
   {REDPIN1, GREENPIN1, BLUEPIN1},
   {REDPIN2, GREENPIN2, BLUEPIN2},
   {REDPIN3, GREENPIN3, BLUEPIN3},
   {REDPIN4, GREENPIN4, BLUEPIN4}
};

// State

int *ledToLockBank = (int *)malloc(sizeof(LEDBANKS));
int *lockState = (int *)malloc(sizeof(LOCKBANKS));
int *lockSwState = (int *)malloc(sizeof(LOCKSWBANKS));
int *lockOnClose = (int *)malloc(sizeof(LOCKBANKS));
long *ledState = (long *)malloc(_LEN(LEDBANKS, int) * sizeof(long));
int *currentFanSpeed = (int *)malloc(sizeof(FANPORTS));
int reqSw = 1;
int doorOpenLighting = 1;  // When true, opening the doors will cause the LEDs to turn bright white. 
                           // Closing the door will restore them to their prior color.
void setup() {
  Serial.begin(115200);
  // Bluetooth Mate Silver 
  Serial3.begin(115200);

  ledToLockBank[0] = 1;
  ledToLockBank[1] = 1;
  ledToLockBank[2] = 0;
  ledToLockBank[3] = 0;
  
  int i, x;

  // Set LED pin outputs.  
  for(i = 0; i < (_LEN(LEDBANKS, int) / 3); i++) {
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
  digitalWrite(RELAYPIN, LOW);

  digitalWrite(BLUEPIN4, HIGH);
  digitalWrite(REDPIN4, HIGH);
  digitalWrite(GREENPIN4, HIGH);

  // Null out the state variables.
  memset(currentFanSpeed, 0, sizeof(FANPORTS));
  memset(ledState, 0, _LEN(LEDBANKS, int) * sizeof(long));
  memset(lockSwState, 0, sizeof(LOCKSWBANKS));
  memset(lockState, 0, sizeof(LOCKBANKS));
  memset(lockOnClose, 0, sizeof(LOCKBANKS));
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

HardwareSerial *activeSerial;

void loop() {
  int n = 0;
  int i = 0;
  int swState;
  int c;
  
  delay(DEBOUNCE); // Faux-debounce. 
  
  // Get switch states.
  for(i = 0; i < _LEN(LOCKSWBANKS, int); i++) {
    Serial.print("i = ");
    Serial.println(i);
    swState = digitalRead(LOCKSWBANKS[i]);
    
    if (swState != lockSwState[i]) {
      lockSwState[i] = swState;

      if (lockSwState[i] == HIGH) {
        Serial.println("Door opened.");
        Serial3.println("Door opened.");
        if (doorOpenLighting)
          Serial.print("n = ");
          Serial.println(n);
          for(n = 0; n < _LEN(LEDBANKS, int) / 3; n++) {
            if (ledToLockBank[n] == i) {
              setLED(n, DOOROPEN_COLOR);
            }
          }
      } else {
        Serial.println("Door closed.");
        Serial3.println("Door closed.");
        for(n = 0; n < _LEN(LEDBANKS, int) / 3; n++) {
          Serial.print("n = ");
          Serial.println(n);
          if (ledToLockBank[n] == i) {
            setLED(n, ledState[n]);
          }
        }
        if (lockOnClose[i] == 1) {
          Serial.println("Locking after close.");
          digitalWrite(LOCKBANKS[i], HIGH);
          lockOnClose[i] = 0;
        }
      }
    }
  }
 
  if (Serial.available() > 0) {
    activeSerial = &Serial;
    c = Serial.read();
  } else if (Serial3.available() > 0) {
    Serial.println("B"); // Inform listener there was a Bluetooth command executed    
    c = Serial3.read();
    activeSerial = &Serial3;
  } else {
    c = -1;
  }
  
  if (c > -1) {
    int r;
    char bank = 0;
    
    switch (c) {
      case 'c': // Set LED Bank Color: c[bank][rrggbb]  eg. c0ffffff
        char colorBuff[8];
        char bankBuff[2];
        long color;
        
        r = activeSerial->readBytes(colorBuff, 7);
        colorBuff[7] = 0;
        
        bankBuff = {colorBuff[0], 0};
        bank = strtol(bankBuff, NULL, 10);          
        color = strtol(&colorBuff[1], NULL, 16);
        
        ledState[bank] = color;        
        
        // Activate the color if the door is closed,
        // otherwise it will get activated on next closure.      
       if (!lockSwState[ledToLockBank[bank]] || !doorOpenLighting)
          setLED(bank, color);
        
        break;
      case 'g': // Get info
        int x;
        Serial.print("Sizeof: ");
        Serial.println(_LEN(LEDBANKS, int) / 3);
        Serial.println(sizeof(int));
        
        for(x = 0; x < _LEN(FANPORTS, int); x++) {
          activeSerial->print("f");
          activeSerial->print(x, DEC);
          activeSerial->println(currentFanSpeed[x], DEC);
        }
        for(x = 0; x < _LEN(LOCKSWBANKS, int); x++) {
          activeSerial->print("lss");
          activeSerial->print(x, DEC);
          activeSerial->println(lockSwState[x], DEC);
        }
        break;
      case 'f': // Fan Control: f[bank][speed in hex]  eg. f1ff
        char opts[4];
        int fanSpeed;
      
        r = activeSerial->readBytes(opts, 3);
        opts[3] = 0;
      
        bank = opts[0] - 48;
        fanSpeed = strtol(&opts[1], NULL, 16);
        
        currentFanSpeed[bank] = fanSpeed;
        analogWrite(FANPORTS[bank], fanSpeed);
      
        break;
      case 'l': // Lock Bank: l[bank]  eg. l1
        activeSerial->readBytes(&bank, 1);        
        
        if (!lockSwState[bank-48] || !reqSw) {
          digitalWrite(LOCKBANKS[bank-48], HIGH);
          lockState[bank-48] = 1;
        }
        break;
      case 'U': // Unlock Bank and set Lock On Close, same syntax as u
        activeSerial->readBytes(&bank, 1);
        digitalWrite(LOCKBANKS[bank-48], LOW);
        lockOnClose[bank-48] = 1;
        lockState[bank-48] = 0;
        break;
      case 'u': // Unlock Bank: u[bank]  eg. u0
        activeSerial->readBytes(&bank, 1);
        digitalWrite(LOCKBANKS[bank-48], LOW);
        lockState[bank-48] = 0;
        break;
      case 'r': // "press" switch connected to relay.
        Serial.print("toggling relay");
        digitalWrite(RELAYPIN, HIGH);
        delay(RELAY_DELAY);
        digitalWrite(RELAYPIN, LOW);
        break;
      case 'd': // Toggle door lighting
         doorOpenLighting = !doorOpenLighting;
         break;
      case 'x': // Toggle the switch press lock requirement.
        reqSw = !reqSw;
        break;
    }
  }
}

 
