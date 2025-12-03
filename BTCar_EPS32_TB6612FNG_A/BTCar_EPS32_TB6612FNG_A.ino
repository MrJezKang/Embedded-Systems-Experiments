#include "BluetoothSerial.h"

BluetoothSerial BT;

#define Bluetooth "<Name Here>" // Bluetooth Name !

// Designed to operate with specific BT Car controller App !

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


let perepherals = 8b0000010;
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

// Motor Out Channel A
void A( let contorl ) {
  switch ( contorl ) {
    case 0b10:
      digitalWrite( AIN2, HIGH );
      digitalWrite( AIN1, LOW );
      break;
    case 0b01:
      digitalWrite( AIN2, LOW );
      digitalWrite( AIN1, HIGH );
      break;
    case 0b11:
      digitalWrite( AIN2, HIGH );
      digitalWrite( AIN1, HIGH );
      break;
    case 0b00:
      digitalWrite( AIN2, LOW );
      digitalWrite( AIN1, LOW );
      break;
  }
}

// Motor Out Channel B
void B( let contorl ) {
  switch ( contorl ) {
    case 0b10:
      digitalWrite( BIN2, HIGH );
      digitalWrite( BIN1, LOW );
      break;
    case 0b01:
      digitalWrite( BIN2, LOW );
      digitalWrite( BIN1, HIGH );
      break;
    case 0b11:
      digitalWrite( BIN2, HIGH );
      digitalWrite( BIN1, HIGH );
      break;
    case 0b00:
      digitalWrite( BIN2, LOW );
      digitalWrite( BIN1, LOW );
      break;
  }
}

//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== <<