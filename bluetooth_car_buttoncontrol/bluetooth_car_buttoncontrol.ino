// Bluetooth Car TB6612FNG Control - Low Level
// BTCar_TS_L_v1.0.0ino

#include "BluetoothSerial.h"

#define Bluetooth "BTCar.TS.L.v1.0.0" // Bluetooth Name !

// --- Pin Configuration (Change as needed) ---
// These GPIO pins connect to the TB6612FNG motor driver.
// AIN1 / AIN2  →   motor A direction control
// BIN1 / BIN2  →   motor B direction control
// PWMA / BIN2  →   Motor speed control (PWM)
// STBY         →   Enable Driver Module
#define PWMA 32
#define AIN1 33
#define AIN2 25
#define PWMB 14
#define BIN1 26
#define BIN2 27
#define STBY 13

#define Forward  0b10
#define Backward 0b01
#define Short    0b11
#define Close    0b00

BluetoothSerial BT;

  // F Foward
  // B Backward
  // L Left
  // R Right
  // H Forward-Left
  // G Forward-Right
  // J Backward-Right
  // I Backward-Left
  // S Default BT Value - Stop
const uint8_t MotionTable[] = {
  ['F'] = (Forward  << 2) | Forward,
  ['B'] = (Backward << 2) | Backward,
  ['L'] = (Backward << 2) | Forward,
  ['R'] = (Forward  << 2) | Backward,
  ['H'] = (Close    << 2) | Forward,
  ['G'] = (Forward  << 2) | Close,
  ['J'] = (Close    << 2) | Backward,
  ['I'] = (Backward << 2) | Close,
  ['S'] = (Close    << 2) | Close
};

void setup() {
  Serial.begin( 115200 );
  BT.begin( Bluetooth );

  pinMode( PWMA, OUTPUT );
  pinMode( AIN1, OUTPUT );
  pinMode( AIN2, OUTPUT );
  pinMode( PWMB, OUTPUT );
  pinMode( BIN1, OUTPUT );
  pinMode( BIN2, OUTPUT );
  pinMode( STBY, OUTPUT );

  ledcSetup(0, 20000, 8);    
  ledcAttachPin(PWMA, 0);
  ledcSetup(1, 20000, 8);
  ledcAttachPin(PWMB, 1);

  gpio_set_level( STBY , HIGH );

}

uint8_t command = 'S';

void loop() {

  // Read incoming Bluetooth data
  if ( BT.available() != 0 ) {
    uint8_t data = BT.read();
        if( data != '\n' && data != '\r' && data != ' ' && data != 0 ) { // check if blank
        command = data;   // <-- store and REMEMBER last command
    }
  }
  
  uint8_t motor_bit = MotionTable[command];
  MotorModuleControl( (motor_bit >> 2) & 0b11 , motor_bit & 0b11 );

  ledcWrite( 0 , 255 );
  ledcWrite( 1 , 255 );

  delay(1);

}

void MotorModuleControl(uint8_t A, uint8_t B) {
    DirectionControl(A, AIN1, AIN2);
    DirectionControl(B, BIN1, BIN2);
}
void DirectionControl(uint8_t crtl, uint8_t in1, uint8_t in2) {
    gpio_set_level(in1, ( crtl >> 1 ) & 1);
    gpio_set_level(in2, crtl & 1);
}
