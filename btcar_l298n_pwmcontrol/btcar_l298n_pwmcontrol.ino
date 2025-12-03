#include "BluetoothSerial.h"

BluetoothSerial BT;

// ============================ P I N   D E F I N I T I O N S ============================ 
#define ENA 13 // PWM Pin for Motor A
#define IN1 15 // Control Pin 1 for Motor A
#define IN2 32 // Control Pin 2 for Motor A
#define ENB 14 // PWM Pin for Motor B
#define IN3 27 // Control Pin 1 for Motor B
#define IN4 26 // Control Pin 2 for Motor B

// ========================== C O N T R O L   V A R I A B L E S ========================== 

char driverDriveA = 'F';
char targetDriveA = 'F';
char driverDriveB = 'F';
char targetDriveB = 'F';
char targerSteer = 'R';

// ============================ M O T O R   V A R I A B L E S ============================
int throttle = 0;
int steerPercent = 0;
int APWM = 0;
int BPWM = 0;
int speedCap = 255;

// ============================= M O D E   V A R I A B L E S =============================
bool breakActive = false;
bool instantMode = true;

// ===================== R A M P   C O N T R O L   V A R I A B L E S =====================
long lastRampMillis = 0;
const long rampIntervalMs = 5;
long lastPrintMillis = 0;


//============================================================   S   E   T   U   P   ============================================================ >>


void setup() {

  Serial.begin( 115200 );
  BT.begin( "BTCar_GenA_LA" );

  pinMode( ENA  , OUTPUT );
  pinMode( IN1 , OUTPUT );
  pinMode( IN2 , OUTPUT );
  pinMode( ENB , OUTPUT );
  pinMode( IN3 , OUTPUT );
  pinMode( IN4 , OUTPUT );

  Serial.println( " <---- BTCar_GenA_LA ----> " );

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
      if( data == 'W' ) breakActive = true;
      else if( data == 'w' ) breakActive = false;
      if( data == 'Z' ) instantMode = !instantMode;

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
  int targetENA = map( throttle , 0 , 99 , 0 , speedCap );
  int targetPWMB = map( throttle , 0 , 99 , 0 , speedCap );
  int targetSteerPercent = map( steerPercent , 0 , 60 , 0 , speedCap );

  // Adjust motor speeds based on steering direction
  if( targerSteer == 'R' ) {
    targetPWMB = targetPWMB - targetSteerPercent;
    targetENA = targetENA + targetSteerPercent;
  } else if ( targerSteer == 'L' ) {
    targetENA = targetENA - targetSteerPercent;
    targetPWMB = targetPWMB + targetSteerPercent;
  }

  // Handle reverse direction
  if( targetENA < 0 ) {
    targetDriveA = 'B';
    targetENA = targetENA * -1;
  }
  if( targetPWMB < 0 ) {
    targetDriveB = 'B';
    targetPWMB = targetPWMB * -1;
  }

  // Cap at 255
  if( targetENA >= speedCap ) targetENA  = speedCap;
  if( targetPWMB >= speedCap ) targetPWMB = speedCap;

  // Ramp motor speeds
  if( current - lastRampMillis > rampIntervalMs ) {
    transistion( APWM, targetENA , driverDriveA, targetDriveA );
    transistion( BPWM, targetPWMB, driverDriveB, targetDriveB );
    lastRampMillis = current;
  }
// ================ M O T O R   S P E E D   /   S T E E R   C O N T R O L ================ <<

// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== >>
  if( APWM == 0 ) breakActive ? A( 0b11 ) : A( 0b00 );
  else (driverDriveA == 'F' ) ? A( 0b10 ) : A( 0b01 );

  if( BPWM == 0 ) breakActive ? B( 0b11 ) : B( 0b00 );
  else (driverDriveB == 'F' ) ? B( 0b10 ) : B( 0b01 );
  
  analogWrite( ENA  , APWM );
  analogWrite( ENB , BPWM );
// ================== M O T O R   D R I V E   C O N T R O L   B L O C K ================== <<

// ========================= S E R I A L   P R I N T   B L O C K ========================= >>
  // Print data every 100ms
  if( current - lastPrintMillis > 100 ) {
    lastPrintMillis = current;
    // Format : instantMode , breakActive ,
    Serial.print( instantMode + String( " " ) );
    Serial.print( breakActive + String( " " ) );

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
  if ( instantMode ) {
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

// Motor A control
void A( int bit ) {
  switch ( bit ) {
    case 0b10:
      digitalWrite( IN2, HIGH );
      digitalWrite( IN1, LOW );
      break;
    case 0b01:
      digitalWrite( IN2, LOW );
      digitalWrite( IN1, HIGH );
      break;
    case 0b11:
      digitalWrite( IN2, HIGH );
      digitalWrite( IN1, HIGH );
      break;
    case 0b00:
      digitalWrite( IN2, LOW );
      digitalWrite( IN1, LOW );
      break;
  }
}

// Motor B control
void B( int bit ) {
  switch ( bit ) {
    case 0b10:
      digitalWrite( IN4, HIGH );
      digitalWrite( IN3, LOW );
      break;
    case 0b01:
      digitalWrite( IN4, LOW );
      digitalWrite( IN3, HIGH );
      break;
    case 0b11:
      digitalWrite( IN4, HIGH );
      digitalWrite( IN3, HIGH );
      break;
    case 0b00:
      digitalWrite( IN4, LOW );
      digitalWrite( IN3, LOW );
      break;
  }
}

//====================================================   F   U   N   C   T   I   O   N   S   ==================================================== <<