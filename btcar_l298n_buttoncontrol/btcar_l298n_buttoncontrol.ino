#include "BluetoothSerial.h"

#define Bluetooth "BTCar.LS.v1.0.1" // Bluetooth Name !

// --- Pin Configuration (Change as needed) ---
// These GPIO pins connect to the L298N motor driver.
// n1 / n2  → Left motor direction control
// n3 / n4  → Right motor direction control
// ENA / ENB → Motor speed control (PWM)
#define n1 33
#define n2 25
#define n3 26
#define n4 27
#define ENA 32
#define ENB 14

BluetoothSerial BT;

void setup() {
  Serial.begin( 115200 );
  BT.begin( Bluetooth );

  pinMode( n1, OUTPUT );
  pinMode( n2, OUTPUT );
  pinMode( n3, OUTPUT );
  pinMode( n4, OUTPUT );
  pinMode( ENA, OUTPUT );
  pinMode( ENB, OUTPUT );
}

char command = 'S';

void loop() {

  // Read incoming Bluetooth data
  if ( BT.available() ) {
    char data = BT.read();
        if( data != '\n' && data != '\r' && data != ' ' && data != 0 ) { // check if blank
        command = data;   // <-- store and REMEMBER last command
    }
  }

  switch (command) {

    case 'F': // Forward
      n1_on(); n2_off(); n3_on(); n4_off();
      break;

    case 'B': // Back
      n1_off(); n2_on(); n3_off(); n4_on();
      break;

    case 'L': // Left turn
      n1_off(); n2_on(); n3_on(); n4_off();
      break;

    case 'R': // Right turn
      n1_on(); n2_off(); n3_off(); n4_on();
      break;

    case 'G': // Forward-Left
      n1_off(); n2_off(); n3_on(); n4_off();
      break;

    case 'H': // Forward-Right
      n1_on(); n2_off(); n3_off(); n4_off();
      break;

    case 'I': // Back-Right
      n1_off(); n2_off(); n3_off(); n4_on();
      break;

    case 'J': // Back-Left
      n1_off(); n2_on(); n3_off(); n4_off();
      break;

    default: // Stop
      n1_off(); n2_off(); n3_off(); n4_off();
      break;
  }

    analogWrite( ENA , 255 ); // Set motor speed (0-255)
    analogWrite( ENB , 255 ); // Set motor speed (0-255)
}

void n1_on(){ digitalWrite( n1 , HIGH ); }
void n1_off(){ digitalWrite( n1 , LOW ); }

void n2_on(){ digitalWrite( n2 , HIGH ); }
void n2_off(){ digitalWrite( n2 , LOW ); }

void n3_on(){ digitalWrite( n3 , HIGH ); }
void n3_off(){ digitalWrite( n3 , LOW ); }

void n4_on(){ digitalWrite( n4 , HIGH ); }
void n4_off(){ digitalWrite( n4 , LOW ); }
