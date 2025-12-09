#include "BluetoothSerial.h"

#define Bluetooth "BTDrive.Button_Control.v1.1.0" // Bluetooth Name !

BluetoothSerial BT;

#define PWMA 27
#define AIN1 4
#define AIN2 19
#define PWMB 14
#define BIN1 21
#define BIN2 32
#define STBY 33

#define PWM 225

#define Forward  0b10
#define Backward 0b01
#define Short    0b11
#define Close    0b00

uint8_t command = 'S';
uint8_t defaultSpeed = 0;
long lastTime = 0;

//Bit Packing
// 0 >> Speed Mode : Switch High to Low Speeds
uint8_t prerapherals = 0;

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

  pinMode( 13 , OUTPUT );

  digitalWrite( STBY , HIGH );
  digitalWrite( 13 , HIGH );
  delay( 1000 );

}

void loop() {

  // Read incoming Bluetooth data
  if ( BT.available() ) {
    uint8_t data = BT.read();
    if( data != '\n' && data != '\r' && data != ' ' && data != 0 ) {

      if( data == 'Z' ) togglebit( prerapherals , 0 );

      if( data == 'F' || data == 'B' || data == 'L' || data == 'R' ||
          data == 'H' || data == 'G' || data == 'J' || data == 'I' ||
          data == 'S' ) command = data;

      digitalWrite( 13 , HIGH ); // turn on LED when data received

    }
  } else digitalWrite( 13 , LOW ); // turn off LED when no data

  uint8_t motor_bit = MotionTable(command);
  
  if( getBit( prerapherals , 0 ) ) defaultSpeed = PWM;
  else defaultSpeed = PWM / 3;
  
  MotorModuleControl( (motor_bit >> 2) & 0b11 , motor_bit & 0b11 );

  analogWrite( PWMA , defaultSpeed );
  analogWrite( PWMB , defaultSpeed );

}

  // F Foward
  // B Backward
  // L Left
  // R Right
  // G Forward-Left
  // H Forward-Right
  // I Backward-Right
  // J Backward-Left
  // S Default BT Value - Stop
uint8_t MotionTable( uint8_t command ) {
  switch( command ) {
    case 'F': return ( Forward  << 2 ) | Forward;
    case 'B': return ( Backward << 2 ) | Backward;
    case 'L': return ( Backward << 2 ) | Forward;
    case 'R': return ( Forward  << 2 ) | Backward;
    case 'G': return ( Close    << 2 ) | Forward;
    case 'H': return ( Forward  << 2 ) | Close;
    case 'I': return ( Close    << 2 ) | Backward;
    case 'J': return ( Backward << 2 ) | Close;
    case 'S': return ( Close    << 2 ) | Close;
    default: return  ( Close    << 2 ) | Close;
  }
};

void setBit(uint8_t &byte, uint8_t bitPos) { byte = byte | 1 << bitPos; }
void clearBit(uint8_t &byte, uint8_t bitPos) { byte = byte & ~(1 << bitPos); }
void togglebit(uint8_t &byte, uint8_t bitPos) { byte = byte ^ (1 << bitPos); }
bool getBit(uint8_t byte, uint8_t bitPos) { return (byte >> bitPos) & 1; }

void MotorModuleControl(uint8_t A, uint8_t B) {
    DirectionControl(A, AIN1, AIN2);
    DirectionControl(B, BIN1, BIN2);
}
void DirectionControl(uint8_t crtl, uint8_t in1, uint8_t in2) {
    digitalWrite(in1, ( crtl >> 1 ) & 1);
    digitalWrite(in2, crtl & 1);
}
