#include "BluetoothSerial.h"

#define Bluetooth "BTCar.Btn.v1.0.0" // Bluetooth Name !

#define PWMA 27
#define AIN1 4
#define AIN2 19
#define PWMB 14
#define BIN1 21
#define BIN2 32
#define STBY 33

#define Forward  0b10
#define Backward 0b01
#define Short    0b11
#define Close    0b00

#define ledOn 13

long lastTime = 0;

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
uint8_t MotionTable( uint8_t command ) {
  switch( command ) {
    case 'F': return ( Forward  << 2 ) | Forward;
    case 'B': return ( Backward << 2 ) | Backward;
    case 'L': return ( Backward << 2 ) | Forward;
    case 'R': return ( Forward  << 2 ) | Backward;
    case 'H': return ( Close    << 2 ) | Forward;
    case 'G': return ( Forward  << 2 ) | Close;
    case 'J': return ( Close    << 2 ) | Backward;
    case 'I': return ( Backward << 2 ) | Close;
    case 'S': return ( Close    << 2 ) | Close;
    default: return  ( Close    << 2 ) | Close;
  }
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

  pinMode( ledOn , OUTPUT );

  digitalWrite( STBY , HIGH );
  digitalWrite( ledOn , HIGH );

}

uint8_t command = 'S';

void loop() {

  if( millis() - lastTime > 500 ) {
    lastTime = millis();
    digitalWrite( ledOn , !digitalRead( ledOn ) ); // blink LED
  }

  // Read incoming Bluetooth data
  if ( BT.available() ) {
    uint8_t data = BT.read();
        if( data != '\n' && data != '\r' && data != ' ' && data != 0 ) {
        command = data;
    }
  }
  
  uint8_t motor_bit = MotionTable(command);
  MotorModuleControl( (motor_bit >> 2) & 0b11 , motor_bit & 0b11 );

  analogWrite( PWMA , 255 );
  analogWrite( PWMB , 255 );

}

void MotorModuleControl(uint8_t A, uint8_t B) {
    DirectionControl(A, AIN1, AIN2);
    DirectionControl(B, BIN1, BIN2);
}
void DirectionControl(uint8_t crtl, uint8_t in1, uint8_t in2) {
    digitalWrite(in1, ( crtl >> 1 ) & 1);
    digitalWrite(in2, crtl & 1);
}
