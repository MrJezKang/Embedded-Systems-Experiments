#include "BluetoothSerial.h"

BluetoothSerial BT;

#define Bluetooth "<Name Here>" // Bluetooth Name !

// Designed to operate with specific BT Car controller App and ESP32

// ============================ P I N   D E F I N I T I O N S ============================ 
#define PWMA 13 
#define AIN1 15 
#define AIN2 32 
#define PWMB 14 
#define BIN1 27 
#define BIN2 26 
#define STBY 

// ========================== C O N T R O L   V A R I A B L E S ========================== 

char driverDriveA = 'F';
char targetDriveA = 'F';
char driverDriveB = 'F';
char targetDriveB = 'F';
char targerSteer = 'R';


let prepherals = 8b0000010;
// parkingBreak , speedMode , headLights
bool parkingBreak = false;
bool speedMode = false;
bool headLights = false;
bool canTurnOnSpot = true; 

int throttle = 0;
int steerPercent = 0;
int APWM = 0;
int BPWM = 0;
int speedCap = 255;

// ===================== R A M P   C O N T R O L   V A R I A B L E S =====================
long lastRampMillis = 0;
long rampIntervalsControl = 5;
long lastPrintMillis = 0;


//============================================================   S   E   T   U   P   ============================================================ >>


void setup() {
  
  Serial.begin( 115200 );
  BT.begin( Bluetooth );

  pinMode( PWMA  , OUTPUT );
  pinMode( AIN1 , OUTPUT );
  pinMode( AIN2 , OUTPUT );
  pinMode( PWMB , OUTPUT );
  pinMode( BIN1 , OUTPUT );
  pinMode( BIN2 , OUTPUT );
  pinMode( STBY , OUTPUT );

  digitalWrite( STBY , HIGH );

}


//============================================================   S   E   T   U   P   ============================================================ <<
//================================================================   L   O   O   P   ============================================================ >>


void loop() {
// ======================= B L U E T O O T H   D A T A   B L O C K ======================= >>
  if(BT.available()) {
    char data = BT.read();

    // ============================== D A T A   ! B L A N K ==============================
    if( data != '\n' && data != '\r' && data != ' ' && data != 0 ) {

      

      // ============================= L O G I C   C H E C K ============================= >>
      if( data == 'W' ) parkingBreak = true;
      else if( data == 'w' ) parkingBreak = false;
      if( data == 'Z' ) speedMode = !speedMode;

      // ============================ G E T   M O V E M E N T ============================ >>
      if( data == 'F' || data == 'B' ) {

        targetDriveA = data; 
        targetDriveB = data;

        String placeHolder = "";

        for( int i = 0 ; i < 2 ; i++ ) {
          char x = BT.read();
          placeHolder = placeHolder + x;
        }

        throttle = placeHolder.toInt();

        placeHolder = "";

        data = BT.read();

        targerSteer = data;

        for( int i = 0 ; i < 2 ; i++ ) {
          char x = BT.read();
          placeHolder = placeHolder + x;
        }

        steerPercent = placeHolder.toInt();

      }
      // ============================ G E T   M O V E M E N T ============================ <<
      
    }

  }
// ======================= B L U E T O O T H   D A T A   B L O C K ======================= <<
  
  long current = millis();

// ================ M O T O R   S P E E D   /   S T E E R   C O N T R O L ================ >>
  int targetPWMA = map( throttle , 0 , 99 , 0 , speedCap );
  int targetPWMB = map( throttle , 0 , 99 , 0 , speedCap );
  int targetSteerPercent = map( steerPercent , 0 , 60 , 0 , speedCap );

  // Adjust motor speeds based on steering direction
  if( targerSteer == 'R' ) {
    targetPWMB = targetPWMB - targetSteerPercent;
    targetPWMA = targetPWMA + targetSteerPercent;
  } else if ( targerSteer == 'L' ) {
    targetPWMA = targetPWMA - targetSteerPercent;
    targetPWMB = targetPWMB + targetSteerPercent;
  }

  // Handle reverse direction
  if( targetPWMA < 0 ) {
    targetDriveA = 'B';
    targetPWMA = targetPWMA * -1;
  }
  if( targetPWMB < 0 ) {
    targetDriveB = 'B';
    targetPWMB = targetPWMB * -1;
  }

  // Cap at 255
  if( targetPWMA >= speedCap ) targetPWMA  = speedCap;
  if( targetPWMB >= speedCap ) targetPWMB = speedCap;

  // Ramp motor speeds
  if( current - lastRampMillis > rampIntervalsControl ) {
    transistion( APWM, targetPWMA , driverDriveA, targetDriveA );
    transistion( BPWM, targetPWMB, driverDriveB, targetDriveB );
    lastRampMillis = current;
  }
// ================ M O T O R   S P E E D   /   S T E E R   C O N T R O L ================ <<

// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== >>
  if( APWM == 0 ) parkingBreak ? A( 0b11 ) : A( 0b00 );
  else (driverDriveA == 'F' ) ? A( 0b10 ) : A( 0b01 );

  if( BPWM == 0 ) parkingBreak ? B( 0b11 ) : B( 0b00 );
  else (driverDriveB == 'F' ) ? B( 0b10 ) : B( 0b01 );
  
  analogWrite( PWMA  , APWM );
  analogWrite( PWMB , BPWM );
// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== <<

// ========================= S E R I A L   P R I N T   B L O C K ========================= >>
  // Print data every 100ms
  if( current - lastPrintMillis > 100 ) {
    lastPrintMillis = current;
    // Format : speedMode , parkingBreak ,
    Serial.print( speedMode + String( " " ) );
    Serial.print( parkingBreak + String( " " ) );

    // Format : driverDriveA , driverDriveB , APWM , BPWM , targerSteer , targetSteerPercent
    Serial.print( driverDriveA );
    Serial.print( driverDriveB + String( " " ) );
    Serial.print( ( ( APWM < 10 ) ? ( "00" + String( APWM ) ) : ( APWM < 100 )  ? ( "0" + String( APWM ) ) : String( APWM ) ) + " " );
    Serial.print( ( ( BPWM < 10 ) ? ( "00" + String( BPWM ) ) : ( BPWM < 100 )  ? ( "0" + String( BPWM ) ) : String( BPWM ) ) + " " );
    Serial.print( targerSteer + String( " " ) );
    Serial.print( ( ( targetSteerPercent < 10 ) ? ( "00" + String( targetSteerPercent ) ) : ( targetSteerPercent < 100 )  ? ( "0" + String( targetSteerPercent ) ) : String( targetSteerPercent ) ) + " " );
    Serial.println(); 
  }
// ========================= S E R I A L   P R I N T   B L O C K ========================= >>
}


//================================================================   L   O   O   P   ============================================================ <<
//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== >>

// Ramp function
void transistion( int &PWM, int targetPWM, char &drive, char targetDrive ) {
  if ( speedMode ) {
    drive = targetDrive;
    PWM = targetPWM;
  } else {
    if ( drive == targetDrive ) {
      if ( PWM > targetPWM ) PWM--;
      else if ( PWM < targetPWM ) PWM++;
    } else {
      if ( PWM > 0 ) PWM--;
      else drive = targetDrive;
    }
  }
}

/*  MotorControl() Function – Sets Motor Direction via Motor Driver
    Supports: LN298, TB6612FNG (logic-level input)
    WARNING: DO NOT CONNECT DC MOTOR DIRECTLY TO ESP32 GPIO  

    Parameters:
      control: 2-bit decode for controlling direction
        Use Defined Constrains for ease of Control if existing:
          #define Forward   0b10  >>  A=HIGH, B=LOW
          #define Backward  0b01  >>  A=LOW,  B=HIGH
          #define Short     0b11  >>  A=HIGH, B=HIGH
          #define Coast     0b00  >>  A=LOW,  B=LOW
      ControlA : Motor Driver DC Motor Direction Control In A
      ControlB : Motor Driver DC Motor Direction Control In B

    Usage Example:
      MotorControl(Forward, AIN1, AIN2);  Using Defined Constrains 
      MotorControl(0b10, 15, 32);         Using Raw Input
      MotroControl(Forward, 15, 32);      Using Mixed

    Notes:
      - uint8_t = 8-bit / 1-byte unsigned integer
      - gpio_set_level() = digitalWrite();
      - gpio_set_level() is ESP32-specific; pin must be configured as OUTPUT
      - Faster than digitalWrite();
      - Safe when logic-level inputs only

    Truth Table:
      Control  | Control  A | Control  B |   Action
      -----------------------------------------------
       0b00    |    LOW     |    LOW     |   Coast
       0b01    |    LOW     |    HIGH    |   Backward
       0b10    |    HIGH    |    LOW     |   Forward
       0b11    |    HIGH    |    HIGH    |   Short

    Operation:
      ( Control >> 1 ) & 1; ( <byte> <shift operation> <shift to what index> ) <and operation> <mask> ;
        Bit Shift Control 0b10 > 0b01
        then oprate and operation 0b01 & 0b01 > return 1 == HIGH;
*/
void MotorControl(uint8_t control, uint8_t controlA, uint8_t controlB) {
    gpio_set_level(controlA, ( control >> 1 ) & 1);
    gpio_set_level(controlB, control & 1);
}

//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== <<