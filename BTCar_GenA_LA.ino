#include "BluetoothSerial.h"

BluetoothSerial BT;

#define PWMA 13
#define AIN2 15
#define AIN1 32
#define STBY 14

int throttle = 0;
int steerPercent = 0;
char directions[5];
bool breakActive = false;
bool instantMode = false;
bool isStill = true;
int PWM = 0;
long lastRampMillis = 0;
const long rampIntervalMs = 500;

//====================================================================================================================================================


void setup() {

  Serial.begin ( 115200 );
  BT.begin ( "Chinese.SurvalanceDrone.Thetta-01" );

  pinMode ( PWMA , OUTPUT );
  pinMode ( AIN2 , OUTPUT );
  pinMode ( AIN1 , OUTPUT );
  pinMode ( STBY , OUTPUT );

  digitalWrite ( STBY , HIGH );

  directions[4] = '\0';

  Serial.println ( " <---- Chinese.SurvalanceDrone.Thetta-01 ----> " );

}


//====================================================================================================================================================


void loop() {

//============================== B L U E T O O T H   D A T A   B L O C K =============== >>
  if (BT.available()) {
    char data = BT.read();

    Serial.print( "Inbound : " + String(data) + "\t\t" );
    
    // ========================== D A T A   ! B L A N K ================================
    if ( data != '\n' && data != '\r' && data != ' ' && data != 0 ) {

      // ======================== L O G I C   C H E C K ================================ >>
      if ( data == 'W' ) breakActive = true;
      else if ( data == 'w' ) breakActive = false;
      
      if ( data == 'Z' ) instantMode = !instantMode;
      // ======================== L O G I C   C H E C K ================================ <<

      // ======================== G E T   M O V E M E N T ============================== >>
      if ( data == 'F' || data == 'B' ) {

        directions[0] = data; 

        String throttle_value = "";
        String steerPercent_Value = "";

        for ( int i = 0 ; i < 2 ; i++ ) {
          char x = BT.read();
          throttle_value = throttle_value + x;
        }

        data = BT.read();
        directions[1] = data;

        for ( int i = 0 ; i < 2 ; i++ ) {
          char x = BT.read();
          steerPercent_Value = steerPercent_Value + x;
        }

        throttle = throttle_value.toInt();
        steerPercent = steerPercent_Value.toInt();

      }
      // ======================== G E T   M O V E M E N T ============================== <<

      Serial.print( "Acceleration : " + String( throttle ) + String( directions[0] ) + "\t\t" );
      Serial.print( "steerPercent : " + String( steerPercent ) + String( directions[1] ) + "\t\t" );
      Serial.print( "breakActive : " + String( breakActive ) + "\t\t" );
      Serial.print( "Instant : " + String( instantMode ) + "\t\t" );
      Serial.println();
      
    }

  }
  //============================== B L U E T O O T H   D A T A   B L O C K ============== <<
  
  int targetPwm = map( throttle , 0 , 99 , 0 , 255 );
  int targetSteerPercent = map ( steerPercent , 0 , 60 , 0 , 255 );

  long current = millis();

  //============================== S P E E D   C O N T R O L   B L O C K ================ >>
  if ( breakActive && targetPwm == 0 ) {
    directions[2] = directions[0];
    PWM = 0;
  }
  else if( !instantMode ) {
    if( isStill ) {
      directions[2] = directions[0];
      isStill = ( targetPwm == 0 );
    } else {
      if ( directions[2] == directions[0] ) {
        if( ( current - lastRampMillis
       ) > ( rampIntervalMs ) ) {
          lastRampMillis
         = current;
          if ( PWM > targetPwm ) PWM--;
          else if ( PWM < targetPwm ) PWM++;
        }
      } else if ( directions[2] != directions[0] || targetPwm == 0 ) {
        if ( PWM > 0 ) PWM--;
        else {
          isStill = ( targetPwm == 0 );
          directions[2] = directions[0];
        }
      }
    }
  } else {
    directions[2] = directions[0];
    PWM = targetPwm;
  }
  //============================== S P E E D   C O N T R O L   B L O C K ================ <<

  if ( PWM != 0 ) {
  if ( directions[2] == 'F' ) {
  digitalWrite ( AIN1 , HIGH );
  digitalWrite ( AIN2 , LOW );
  } else { 
    digitalWrite ( AIN1 , LOW ); 
    digitalWrite ( AIN2 , HIGH );
  }
  } else {
    digitalWrite( AIN1 , breakActive ? HIGH : LOW );
    digitalWrite( AIN2 , breakActive ? HIGH : LOW );
  }
 
  analogWrite ( PWMA , PWM );

}


//====================================================================================================================================================