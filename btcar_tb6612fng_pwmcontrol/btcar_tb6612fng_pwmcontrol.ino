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
#define STBY 33

// ========================== C O N T R O L   V A R I A B L E S ========================== 

#define Forward 0b10
#define Backward 0b01
#define Short 0b11
#define Coast 0b00

#define SpeedCap 255

char driverDriveA = 'F';
char targetDriveA = 'F';
char driverDriveB = 'F';
char targetDriveB = 'F';
char targetSteer = 'R';

bool parkingBreak = false;
bool speedMode = false;
bool headLights = false;
bool canTurnOnSpot = true; 

int throttle = 0;
int steerPercent = 0;
int APWM = 0;
int BPWM = 0;
int speed = 0;

// ===================== R A M P   C O N T R O L   V A R I A B L E S =====================
long lastRampMillisA = 0;
long lastRampMillisB = 0;
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

  digitalWrite( STBY , HIGH ); // enable motor driver

  // Motor A PWM
  ledcSetup(0, 20000, 8);      // Channel 0, 20 kHz, 8-bit resolution
  ledcAttachPin(PWMA, 0);      // Attach Motor A PWM pin

  // Motor B PWM
  ledcSetup(1, 20000, 8);      // Channel 1, 20 kHz, 8-bit resolution
  ledcAttachPin(PWMB, 1);      // Attach Motor B PWM pin


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

      speedMode ? speed = SpeedCap : speed = SpeedCap / 2;

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

        targetSteer = data;

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
  int targetPWMA = map( throttle , 0 , 99 , 0 , speed );
  int targetPWMB = map( throttle , 0 , 99 , 0 , speed );
  int targetSteerPercent = map( steerPercent , 0 , 60 , 0 , speed );

  ( targetPWMA > 0 || targetPWMB > 0 ) ? canTurnOnSpot = true : ( ( targetPWMA == 0 && targetPWMB == 0 ) && ( targetSteerPercent == 0 ) ? canTurnOnSpot = true : canTurnOnSpot = false );

  // Adjust motor speeds based on steering direction
  if( targetSteer == 'R' ) {
    targetPWMB = targetPWMB - targetSteerPercent;
    targetPWMA = targetPWMA + targetSteerPercent;
  } else if ( targetSteer == 'L' ) {
    targetPWMA = targetPWMA - targetSteerPercent;
    targetPWMB = targetPWMB + targetSteerPercent;
  }

  // Handle On Spot turn direction
  if ( canTurnOnSpot ) {
    if( targetPWMA < 0 ) {
      targetDriveA = 'B';
      targetPWMA = -targetPWMA;  // make positive
    }
    if( targetPWMB < 0 ) {
      targetDriveB = 'B';
      targetPWMB = -targetPWMB;  // make positive
    }
  } else {
    targetPWMA = constrain(targetPWMA, 0, speed);
    targetPWMB = constrain(targetPWMB, 0, speed);
  }
  

  // Cap at 255
  if( targetPWMA >= speed ) targetPWMA  = speed;
  if( targetPWMB >= speed ) targetPWMB = speed;



  int rampIntervalsControlA = map(abs(APWM), 0, 255, 10, 2);  // closer to 0 → slower
  int rampIntervalsControlB = map(abs(BPWM), 0, 255, 10, 2);

  int rampIntervalsControlA = constrain(rampIntervalsControlA, 2, 10);
  int rampIntervalsControlB = constrain(rampIntervalsControlB, 2, 10);

  // Ramp motor speeds
  if( current - lastRampMillisA > rampIntervalsControlA ) {
    transistion( APWM, targetPWMA , driverDriveA, targetDriveA );
    lastRampMillisA = current;
  }
   if( current - lastRampMillisB > rampIntervalsControlB ) {
    transistion( BPWM, targetPWMB, driverDriveB, targetDriveB );
    lastRampMillisB = current;
  }
// ================ M O T O R   S P E E D   /   S T E E R   C O N T R O L ================ <<

// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== >>
  if ( APWM == 0 && BPWM == 0 ) {
    if ( parkingBreak ) {
      MotorControl( Short , AIN1 , AIN2 );
      MotorControl( Short , BIN1 , BIN2 );
    } else {
      MotorControl( Coast , AIN1 , AIN2 );
      MotorControl( Coast , BIN1 , BIN2 );
    }
  } else {
    (driverDriveA == 'F' ) ? MotorControl( Forward , AIN1 , AIN2 ) : MotorControl( Backward , AIN1 , AIN2 );
    (driverDriveB == 'F' ) ? MotorControl( Forward , BIN1 , BIN2 ) : MotorControl( Backward , BIN1 , BIN2 );
  }
  
  ledcWrite( 0 , APWM );
  ledcWrite( 1 , BPWM );
// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== <<

// ========================= S E R I A L   P R I N T   B L O C K ========================= >>
  // Print data every 100ms
  if( current - lastPrintMillis > 100 ) {
    lastPrintMillis = current;
    // Format : speedMode , parkingBreak ,
    Serial.print( speedMode + String( " " ) );
    Serial.print( parkingBreak + String( " " ) );

    // Format : driverDriveA , driverDriveB , APWM , BPWM , targetSteer , targetSteerPercent
    Serial.print( driverDriveA );
    Serial.print( driverDriveB + String( " " ) );
    Serial.print( ( ( APWM < 10 ) ? ( "00" + String( APWM ) ) : ( APWM < 100 )  ? ( "0" + String( APWM ) ) : String( APWM ) ) + " " );
    Serial.print( ( ( BPWM < 10 ) ? ( "00" + String( BPWM ) ) : ( BPWM < 100 )  ? ( "0" + String( BPWM ) ) : String( BPWM ) ) + " " );
    Serial.print( targetSteer + String( " " ) );
    Serial.print( ( ( targetSteerPercent < 10 ) ? ( "00" + String( targetSteerPercent ) ) : ( targetSteerPercent < 100 )  ? ( "0" + String( targetSteerPercent ) ) : String( targetSteerPercent ) ) + " " );
    Serial.println(); 
  }
// ========================= S E R I A L   P R I N T   B L O C K ========================= >>
}


//================================================================   L   O   O   P   ============================================================ <<
//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== >>

// Ramp function
void transistion( int &PWM, int targetPWM, char &drive, char targetDrive ) {
  if ( drive == targetDrive ) {
    if ( PWM > targetPWM ) PWM--;
    else if ( PWM < targetPWM ) PWM++;
  } else {
    if ( PWM > 0 ) PWM--;
    else drive = targetDrive;
  }
}

void MotorControl( int control , int controlA , int controlB ) {
    digitalWrite( controlA , ( control >> 1 ) & 1 );
    digitalWrite( controlB , control & 1 );
}

//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== <<