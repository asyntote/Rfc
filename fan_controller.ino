/*
 *  Reflection Fan Controller 
 *    !V 0.1-001
 *    !GPL V3
 *    
 *  gKript.org @ 09/2020 (R)
 *    !asyntote
 *    !skymatrix
 *    
 *  @Arduino Due
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastLED.h>
#include "Relay.h"

FASTLED_USING_NAMESPACE

#define RFC_VERSION         "1.7-005"
//  ------------------------------------------------------------------- GENERAL DEFINES
#define __OFF       0
#define __ON        1
#define __FOFF      -1
#define __RESET     -1

//void(* Riavvia)(void) = 0;

#define __INTERNAL    0
#define __TTTERM      1

#define __TERMINAL    __TTTERM

#if ( __TERMINAL == __INTERNAL )
  byte __SCROLL = __ON;
#else
  byte __SCROLL = __OFF;
#endif

#define __NORMAL      if ( __TERMINAL == __TTTERM ) Serial.print( "\e[0m" )
#define __BOLD        if ( __TERMINAL == __TTTERM ) Serial.print( "\e[1m" )

//  ------------------------------------------------------------------- RFC DEFINES
#define THV_ACTIVATION      55.0
#define THV_DEACTIVATION    50.0
#define THE_ACTIVATION      40.0 
#define THE_DEACTIVATION    35.0

#define __ERRORE_STOP       while(1)

unsigned long               cycles;
unsigned long               nrun = 0;
unsigned long               Texe = 0;

int inByte = 0;

#define __ENABLE            0
#define __DISABLE           1

int AutoInit = __OFF;  
int    Force = 0;             //  Force Generale  (accende)
int   SForce = 0;             //  Stop Forzato
int  VEnable = 0;             //  Canale Video
int  MEnable = 0;             //  Canale Main Side
int  OEnable = 0;             //  Canale Other Side
int  NEnable = 0;             //  Canale N

int   inHelp = 0;

int Controller = __ON;

byte delta = __OFF;

//  ------------------------------------------------------------------- SENSOR DEFINES
#define ONE_WIRE_BUS  18
#define SERIAL_DIGIT  2

#define _NUM_SENSORS  3

#define __OLD         1
#define __NEW         2

#define __READING     __NEW

#if ( __READING == __OLD )

  #define RFC_DELAY     500
  #define __RESOLUTION  9
  #define __AVG_READ    4

#elif ( __READING == __NEW )

  #define RFC_DELAY     500
  #define __RESOLUTION  10
  
#endif

#define TH_AVERAGE  25

#define TH_VIDEO    1
#define TH_ENVRM    2
#define TH_AMBNT    0

OneWire             oneWire(ONE_WIRE_BUS);
DallasTemperature   sensors(&oneWire);

DeviceAddress       THvideo;
DeviceAddress       THenvrm;
DeviceAddress       THambnt;

int id = 0;

#define             __AMB_TIME  100
unsigned long       time_amb = __RESET;

float THV_s[ TH_AVERAGE ];
float THE_s[ TH_AVERAGE ];

float aTHA = 0.0;
float aTHV = 0.0;
float aTHE = 0.0;

float mTHV = 0.0;
float mTHE = 0.0;

#define THA_MAX     40.0
#define THA_MIN     25.0

#define THE_MAX     60.0
#define THE_MIN     30.0

#define THV_MAX     60.0
#define THV_MIN     45.0


#define __EQUAL_LIMIT   60    //  uguali tra i vari sensori
#define __SAME_LIMIT    250   //  stessa lettura sul singolo sensore

byte equal = 0;
byte gsame = 0;
float asame = 0;
byte esame = 0;
byte vsame = 0;

void bigFail( void );

void  SensErrorCheck( void ) {
  byte seerror = __OFF;
  static float  otha = 0.0;
  static float  othe = 0.0;
  static float  othv = 0.0;


  if ( otha != aTHA )
    asame+= 0.1;
  else
    asame = 0.0;
  if ( othe != aTHE )
    esame++;
  else
    esame = 0;
  if ( othv != aTHV )
    vsame++;
  else
    vsame = 0;
  gsame = (byte)asame;
  if ( esame > gsame )
    gsame = esame;  
  if ( vsame > gsame )
    gsame = vsame;  
  if ( gsame > __SAME_LIMIT )
    seerror = __ON;

  otha = aTHA;
  othe = aTHE;
  othv = aTHV;
      
  if ( ( aTHA == aTHE ) || ( aTHE == aTHV ) || ( aTHV == aTHA ) ) {
    equal++;
  } else {
    equal = 0;
  }
  if ( equal >= __EQUAL_LIMIT ) {
    seerror = __ON;
  }
  if( ( aTHA < 0 ) || ( aTHE < 0 ) || ( aTHV < 0 ) ) {
    seerror = __ON;
  }
  if ( seerror == __ON ) {
    Serial.println( "" );
    Serial.println( "Something went wrong!!!" );
    Serial.println( "!!!Reset!!!" );
    bigFail();
  }
}

//  ------------------------------------------------------------------- RELAY DEFINES
#define RELAY_DLY   250

#define __VIDEO     7
#define __MAIN_SIDE 8
#define __OTHR_SIDE 9
#define __NOT_USED  10

#define __NORMALY_OPENED  true
#define __NORMALY_CLOSED  false

Relay   Video     ( __VIDEO     , __NORMALY_OPENED );
Relay   MainPanel ( __MAIN_SIDE , __NORMALY_OPENED );
Relay   OtherPanel( __OTHR_SIDE , __NORMALY_OPENED );

byte Rvd = __OFF;
byte Rmp = __OFF;
byte Rop = __OFF;
byte Rnu = __OFF;

//  ------------------------------------------------------------------- MMACROS FROM DateTime.h
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)


//  ------------------------------------------------------------------- FASTLED DEFINES
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
  #warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    16
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    10
#define BRIGHTNESS  64

CRGB leds[NUM_LEDS];

#define _LN_SV      0
#define _LN_SE      1
#define _LN_SA      2
#define _LN_SNU     3
#define _LN_CNTR    4
#define _LN_VV      5
#define _LN_VM      6
#define _LN_VO      7
#define _LN_VNU     8
#define _LN_ON      9

#define _L_0XFF     64

#define _L_ERROR_ON(a)    leds[ a ].setRGB( _L_0XFF, 0, 0 )
#define _L_ERROR_OFF(a)   leds[ a ].setRGB( 0, 0, 0 )

#define _L_ON_ON          leds[9].setRGB( 0, _L_0XFF, 0)
#define _L_ON_OFF         leds[9].setRGB( 0, 0, 0)

#define _L_CNTR_OFF       leds[4].setRGB( 0, 0, 0)
#define _L_CNTR_INIT      leds[4].setRGB( _L_0XFF, _L_0XFF, 0)
#define _L_CNTR_MANU      leds[4].setRGB( _L_0XFF, 0, 0)
#define _L_CNTR_AUTO      leds[4].setRGB( 0, _L_0XFF, 0)

#define _L_VO_ON          leds[7].setRGB( 0, _L_0XFF, 0)
#define _L_VO_OFF         leds[7].setRGB( 0, 0, 0)
#define _L_VO_DIS         leds[7].setRGB( _L_0XFF, 0, 0)

#define _L_VM_ON          leds[6].setRGB( 0, _L_0XFF, 0)
#define _L_VM_OFF         leds[6].setRGB( 0, 0, 0)
#define _L_VM_DIS         leds[6].setRGB( _L_0XFF, 0, 0)

#define _L_VV_ON          leds[5].setRGB( 0, _L_0XFF, 0)
#define _L_VV_OFF         leds[5].setRGB( 0, 0, 0)
#define _L_VV_DIS         leds[5].setRGB( _L_0XFF, 0, 0)

#define _L_SA_OFF         leds[2].setRGB( 0, 0, 0)
#define _L_SA_SET(a,b,c)  leds[2].setRGB( a, b, c)

#define _L_SE_OFF         leds[1].setRGB( 0, 0, 0)
#define _L_SE_SET(a,b,c)  leds[1].setRGB( a, b, c)

#define _L_SV_OFF         leds[0].setRGB( 0, 0, 0)
#define _L_SV_SET(a,b,c)  leds[0].setRGB( a, b, c)

#define _L_NV8_OFF        leds[8].setRGB( 0, 0, 0)
#define _L_NS3_OFF        leds[3].setRGB( 0, 0, 0)

#define _L_SET(l,a,b,c)   leds[ l ].setRGB( a, b, c)
#define _L_UPDATE         FastLED.show()

#define _ALL_LED_OFF      for( int iii = 0 ; iii < NUM_LEDS ; iii++ ) { \  
                            leds[ iii ].setRGB( 0, 0, 0);               \
                          }                                             \
                          FastLED.show();                               \

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (float)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

#define _R_MAX    255.0
#define _B_MAX    255.0

void  set_ls_temp( byte led , float temp ) {
  int rv = 0.0;
  int bv = 0.0;
  if ( led == _LN_SV ) {
    rv = (int)fmap( temp , THV_MIN , THV_MAX , 0.0 , _R_MAX );
    bv = (int)fmap( temp , THV_MIN , THV_MAX , 0.0 , _B_MAX );
  } else if ( led == _LN_SE ) {
    rv = (int)fmap( temp , THE_MIN , THE_MAX , 0.0 , _R_MAX );
    bv = (int)fmap( temp , aTHA    , THE_MAX , 0.0 , _B_MAX );
  } else if ( led == _LN_SA ) {
    rv = (int)fmap( temp , THA_MIN , THA_MAX , 0.0 , _R_MAX );
    bv = (int)fmap( temp , THA_MIN , THA_MAX , 0.0 , _B_MAX );
  }
  bv = 255 - bv; 
  if ( rv < 0 ) rv = 0;
  if ( rv > _R_MAX ) rv = _R_MAX;
  if ( bv < 0 ) bv = 0;
  if ( bv > _B_MAX ) bv = _B_MAX;
  _L_SET( led , rv , 0 , bv );
}


//  ------------------------------------------------------------------- TIME CODE !!!
byte hours = 0;
byte minutes = 0;
byte seconds = 0;

void time(unsigned long val){  
  hours = numberOfHours(val);
  minutes = numberOfMinutes(val);
  seconds = numberOfSeconds(val);
}

void printDigits(byte digits) {
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits,DEC);  
}


void  printStatus( void ) {
  if( __TERMINAL == __INTERNAL ) {
    Serial.print("Control: ");
    if ( AutoInit == __OFF )
      Serial.print("INIT   ");
    else {
      if ( Controller == __ON ) 
        Serial.print("AUTO   ");
      else
        Serial.print("MANUAL ");
    }
    if ( VEnable == __ENABLE )
      Serial.print( Rvd?"V":"-" );
    else 
      Serial.print( "d" );  
    if ( MEnable == __ENABLE )  
      Serial.print( Rmp?"M":"-" );
    else 
      Serial.print( "d" );  
    if ( OEnable == __ENABLE )
      Serial.print( Rop?"O":"-" );
    else 
      Serial.print( "d" );  
    if ( NEnable == __ENABLE )
      Serial.print( Rnu?"N":"-" );
    else 
      Serial.print( "d" );
  }
  else {
    Serial.print("Control: ");
    if ( AutoInit == __OFF )
      Serial.print("INIT   ");
    else {
      if ( Controller == __ON ) 
        Serial.print("\e[1mAUTO\e[0m   ");
      else
        Serial.print("\e[1mMANUAL\e[0m ");
    }
    if ( VEnable == __ENABLE )
      Serial.print( Rvd?"\e[1mV\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[1m" );  
    if ( MEnable == __ENABLE )  
      Serial.print( Rmp?"\e[1mM\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );  
    if ( OEnable == __ENABLE )
      Serial.print( Rop?"\e[1mO\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );  
    if ( NEnable == __ENABLE )
      Serial.print( Rnu?"\e[1mN\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );
  }
}

//  ------------------------------------------------------------------- CODE START
void bigFail( void ) {
  byte r = 0;
  _ALL_LED_OFF;
  __ERRORE_STOP {
    checkSer();
    _L_ERROR_ON( _LN_ON );
    _L_ERROR_ON( _LN_CNTR );
    _L_UPDATE;
    delay( 750 );
    _L_ERROR_OFF( _LN_ON );
    _L_ERROR_OFF( _LN_CNTR );
    _L_UPDATE;
    delay( 750 );
    if ( r++ > 200 )
      rstc_start_software_reset(RSTC);
  }  
}


void SerDebug( void ) {

  if ( delta == __OFF ) {
    if ( Texe ) {
      if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
        Serial.println("\e[1K");
        Serial.print("\e[1A");
      }
      Serial.print(" CH A/E/V: ");
      __BOLD;
      Serial.print(aTHA , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHE , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHV , SERIAL_DIGIT );
      __NORMAL;
      Serial.print(" - Me/Mv: ");
      __BOLD;
      Serial.print(mTHE , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(mTHV , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("    -  ");
      printStatus();
      Serial.print("  -  ");
      Serial.print("Cmd: ");
      if ( inByte ) {
        Serial.print( (char)inByte );
      }
      else
        Serial.print( " " );
      Serial.print("  -  ");
      Serial.print("UP time: ");
      printDigits(hours);
      Serial.print(":");
      printDigits(minutes);
      Serial.print(":");
      printDigits(seconds);
      Serial.print("  -  ");
      Serial.print("ms: ");
      Serial.print( Texe );
    }
  }
  else if ( delta == __ON ) {
    if ( Texe ) {
      if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
        Serial.println("\e[1K");
        Serial.print("\e[1A");
      }
      Serial.print(" CH A/dE/dV: ");
      __BOLD;
      Serial.print(aTHA , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHE - aTHA , SERIAL_DIGIT );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHV - aTHA , SERIAL_DIGIT );
      __NORMAL;
      Serial.print(" - Eq/Sm-a-e-v: ");
      __BOLD;
      Serial.print(equal);
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(gsame);
      Serial.print("-");
      Serial.print(asame , 1);
      Serial.print("-");
      Serial.print(esame);
      Serial.print("-");
      Serial.print(vsame);
      __NORMAL;
      Serial.print("  -  ");
      printStatus();
      Serial.print("  -  ");
      Serial.print("Cmd: ");
      if ( inByte ) {
        Serial.print( (char)inByte );
      }
      else
        Serial.print( " " );
      Serial.print("  -  ");
      Serial.print("ms: ");
      Serial.print( ( millis() - time_amb ) );
    }
  }
  if ( inHelp ) {
    Serial.println();
    showHelp();
    inHelp = 0;
  }
  if ( __SCROLL == __ON )
    Serial.println();
}


void  showHelp( void ) {
  Serial.println( "" );
  if ( cycles > 0 ) {
    if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
      Serial.println( "\e[1J" );
    }
    Serial.println( "---------------------------------------------------" );
    Serial.print  ( "Reflection Fan Controller [RFC v");
    Serial.print  ( RFC_VERSION );
    Serial.print  ( "]");
    Serial.println( "" );
    Serial.println( "---------------------------------------------------" );
    Serial.println( "Command   Meaning" );
    Serial.println( "  h/H     This help list" );
    Serial.println( "  R       Push the Reset button" );
    Serial.println( "  a/A     It goes in automatic modality" );
    Serial.println( "  f       It goes in manual modality and will STOP all fans" );
    Serial.println( "  F       It goes in manual modality and will START all fans" );
    Serial.println( "  V       It will enable the Video Fan (default)" );
    Serial.println( "  v       It will disable the Video Fan" );
    Serial.println( "  M       It will enable the Main Panel Fan (default)" );
    Serial.println( "  m       It will disable the Main Panel Fan" );
    Serial.println( "  O       It will enable the Other Panel Fan (default)" );
    Serial.println( "  o       It will disable the Other Panel Fan" );
    delay( 5000 );
    Serial.println();
  }
  else
   Serial.println( "  Press h o H to get the commands list" );
}


 void printAddress(DeviceAddress deviceAddress){
  for (byte i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) 
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void  Relay_manager( int ch , int st ) {
  byte mod = 0;
  switch ( ch ) {
    case __VIDEO : {
      if ( Rvd != st ) {
        Rvd = st;
        if ( st == __ON ) {
          //quadRelay.turnRelayOn(  __VIDEO );
        } else {
          //quadRelay.turnRelayOff( __VIDEO );
        }
        mod = 1;
      }
      break;
    }
    case __MAIN_SIDE : {
      if ( Rmp != st ) {
        Rmp = st;
        if ( st == __ON ) {
          //quadRelay.turnRelayOn(  __MAIN_SIDE );
          _L_VM_ON;
        } else {
          //quadRelay.turnRelayOff( __MAIN_SIDE );
          _L_VM_OFF;
        }
        mod = 1;
      }
      break;
    }
    case __OTHR_SIDE : {
      if ( Rop != st ) {
        Rop = st;
        if ( st == __ON ) {
          //quadRelay.turnRelayOn(  __OTHR_SIDE );
          _L_VO_ON;
        } else {
          //quadRelay.turnRelayOff( __OTHR_SIDE );
          _L_VO_OFF;
        }
        mod = 1;
      }
      break;
    }
    case __NOT_USED : {
      if ( Rnu != st ) {
        Rnu = st;
        if ( st == __ON ) {
          //quadRelay.turnRelayOn(  __NOT_USED );
        } else {
          //quadRelay.turnRelayOff( __NOT_USED );
        }
        mod = 1;
    }
      break;
    }
  }
  if ( mod )
    delay( RELAY_DLY );
}

#if ( __READING == __OLD ) 

  void avg_reading( void ) {
    for( int a = 0; a < __AVG_READ ; a++ ) {
      sensors.requestTemperatures();
      aTHV += sensors.getTempC( THvideo );
      aTHE += sensors.getTempC( THenvrm );
    }
    aTHV /= __AVG_READ;
    aTHE /= __AVG_READ;
    if ( time_amb == __RESET )
      time_amb = millis();
    else if ( ( millis() - time_amb ) >= __AMB_TIME ) {
      aTHA = sensors.getTempC( THambnt );
      time_amb = __RESET;
    }
    if ( aTHV > mTHV ) mTHV = aTHV;
    if ( aTHE > mTHE ) mTHE = aTHE;
  }

#elif ( __READING == __NEW ) 

  void avg_reading( void ) {
    sensors.requestTemperatures();
    THV_s[ id ] = sensors.getTempC( THvideo );
    THE_s[ id ] = sensors.getTempC( THenvrm );
    aTHA = sensors.getTempC( THambnt );
    for( int a = 0; a < TH_AVERAGE ; a++ ) {
      aTHV += THV_s[a];
    }
    aTHV /= (float)TH_AVERAGE;
    for( int a = 0; a < TH_AVERAGE ; a++ ) {
      aTHE += THE_s[a];
    }
    aTHE /= (float)TH_AVERAGE;
    if ( ++id >= TH_AVERAGE )
      id = 0;
    if ( aTHV > mTHV ) mTHV = aTHV;
    if ( aTHE > mTHE ) mTHE = aTHE;
  }

#endif

void  Thrm_controller( void ) {
  if ( cycles < (  TH_AVERAGE + 5 ) ) {
    AutoInit = __OFF;
  } else {
    AutoInit = __ON;
//    _L_CNTR_INIT;
  }

  if ( AutoInit == __ON ) {
    if ( ! Force  ) {
      Controller = __ON;
//      _L_CNTR_AUTO;
    } else {
      Controller = __OFF;
//      _L_CNTR_MANU;
    }
      
    if ( Controller == __ON ) {
      Relay_manager( __NOT_USED , __OFF );
      //  ----------------------------------------  TH Video
      if ( aTHV > THV_ACTIVATION ) {
        if ( VEnable == __ENABLE ) Relay_manager( __VIDEO , __ON );
      }
      else {
        if ( aTHV < THV_DEACTIVATION ) {
          if ( VEnable == __ENABLE ) Relay_manager( __VIDEO , __OFF );
        }
      }
  
      //  ----------------------------------------  TH Environment
      if ( aTHE > THE_ACTIVATION ) {
        if ( MEnable == __ENABLE ) Relay_manager( __MAIN_SIDE , __ON );
        if ( OEnable == __ENABLE ) Relay_manager( __OTHR_SIDE , __ON );
      }
      else if ( aTHV > THV_DEACTIVATION ) {
        if ( MEnable == __ENABLE ) Relay_manager( __MAIN_SIDE , __ON );
      }
      else {
        if ( ( aTHE < THE_ACTIVATION ) && ( aTHV < THV_ACTIVATION ) ) {
          if ( OEnable == __ENABLE ) Relay_manager( __OTHR_SIDE , __OFF );
        }
        if ( aTHE < THE_DEACTIVATION ) {
          if ( MEnable == __ENABLE ) Relay_manager( __MAIN_SIDE , __OFF );
        }
      }
    }
    else {
      if ( Force ) {
        Relay_manager( __VIDEO , __ON );
        Relay_manager( __MAIN_SIDE , __ON );
        Relay_manager( __OTHR_SIDE , __ON );
        Relay_manager( __NOT_USED , __ON );
      }
      if ( SForce ) {
        Relay_manager( __VIDEO , __OFF );
        Relay_manager( __MAIN_SIDE , __OFF );
        Relay_manager( __OTHR_SIDE , __OFF );
        Relay_manager( __NOT_USED , __OFF );
      }
    }
  }
}



void checkSer( void ) {
  if (Serial.available()) {
    inByte = Serial.read();
    switch( inByte ) {
      case 'h':
      case 'H': {
        inHelp = 1;
        break;
      }
      case 'R': {
        delay(2000);
        rstc_start_software_reset(RSTC);
        break;
      }
      case 's': {
        __SCROLL = __OFF;
        break;
      }
      case 'S': {
        __SCROLL = __ON;
        break;
      }
      case 'd': {
        delta ^= __ON;
        break;
      }
      case 'a':
      case 'A': {
         Force = 0;
        VEnable = 0;
        MEnable = 0;
        OEnable = 0;
        NEnable = 0;
        SForce = 0;

        break;
      }
      case 'f': {
        VEnable = __DISABLE;
        Relay_manager( __VIDEO , __OFF );
        MEnable = __DISABLE;
        Relay_manager( __MAIN_SIDE , __OFF );
        OEnable = __DISABLE;
        Relay_manager( __OTHR_SIDE , __OFF );

        break;
      }
      case 'F': {
         Force = 1;
        SForce = 0;
        break;
      }
      case 'V':
      case 'v': {
        if ( inByte == 'V' ) VEnable = __ENABLE;
        if ( inByte == 'v' ) {
          Relay_manager( __VIDEO , __OFF );
          VEnable = __DISABLE;
        }
        break;
      }
      case 'M':
      case 'm': {
        if ( inByte == 'M' ) MEnable = __ENABLE;
        if ( inByte == 'm' ) {
          Relay_manager( __MAIN_SIDE , __OFF );
          MEnable = __DISABLE;
        }
        break;
      }
      case 'O':
      case 'o': {
        if ( inByte == 'O' ) OEnable = __ENABLE;
        if ( inByte == 'o' ) {
          Relay_manager( __OTHR_SIDE , __OFF );
          OEnable = __DISABLE;
        }
        break;
      }
      case 'N':
      case 'n': {
        if ( inByte == 'N' ) NEnable = __ENABLE;
        if ( inByte == 'n' ) {
          Relay_manager( __NOT_USED , __OFF );
          NEnable = __DISABLE;
        }
        break;
      }
      default: {
        inByte = 0;
      }
    }
  }
  else
    inByte = 0;
  
}


void LedCntr( void ) {

  if ( VEnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_VV_DIS;
      else
        _L_VV_OFF;
  } else {
    if ( Rvd == __OFF ) {
      _L_VV_OFF;
    } else {
      _L_VV_ON;
    }
  }
  
  if ( MEnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_VM_DIS;
      else
        _L_VM_OFF;
  } else {
    if ( Rmp == __OFF ) {
      _L_VM_OFF;
    } else {
      _L_VM_ON;
    }
  }

  if ( OEnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_VO_DIS;
      else
        _L_VO_OFF;
  } else {
    if ( Rop == __OFF ) {
      _L_VO_OFF;
    } else {
      _L_VO_ON;
    }
  }

  if ( Controller == __ON )
    _L_CNTR_AUTO;
  else {
    if ( nrun % 2 ) {
      _L_CNTR_MANU;
      _L_VM_ON;
      _L_VO_ON;
      _L_VV_ON;
    } else {
      _L_CNTR_OFF;
      _L_VM_OFF;
      _L_VO_OFF;
      _L_VV_OFF;
    }
  }

  if ( AutoInit == __ON ) {
    set_ls_temp( _LN_SV , aTHV );
    set_ls_temp( _LN_SE , aTHE );
    set_ls_temp( _LN_SA , aTHA );
  } else {
    if ( nrun % 2 )
        _L_CNTR_INIT;
      else
        _L_CNTR_OFF;
   }

  if ( gsame > 100 )
    _L_CNTR_INIT;
    
  _L_UPDATE;
}

//  ------------------------------------------------------------------- SETUP
void setup(void) {

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
 
  _ALL_LED_OFF
  _L_ON_ON;
  _L_UPDATE;
  
  for( byte a = 0; a < TH_AVERAGE ; a++ ) 
    THV_s[a] = 0.0;
  for( byte a = 0; a < TH_AVERAGE ; a++ )
    THE_s[a] = 0.0;
  Serial.begin(115200);
  if ( __TERMINAL != __INTERNAL ) {
    for( byte a = 0; a < 40 ; a++ )
      Serial.println();
    Serial.println( "\e[1J" );
    Serial.print("\e[2J");
  }
  Serial.println();
  Serial.println("---------------------------------------------------");
  Serial.println();
  Serial.print  ("Reflection Fan Controller [RFC v");
  Serial.print( RFC_VERSION );
  Serial.print("]");
  Serial.println();


  //  ------------------------------------------------------------------- SENSORE BEGIN
  pinMode( ONE_WIRE_BUS , INPUT_PULLUP );
  Serial.println();
  Serial.print("Thermal Sensor Initializing [DS18B20]...");
  Serial.println();
  Serial.print("  Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  Serial.println();

  if ( sensors.getDeviceCount() == _NUM_SENSORS ) {
    //  ---------
    Serial.println("  Ambient thermal sensor");
    Serial.print("    parasite power is: ");
    if (sensors.isParasitePowerMode())
      Serial.println("ON");
    else
      Serial.println("OFF");
    if (!sensors.getAddress(THambnt, TH_AMBNT))
      Serial.println("Unable to find address for THambnt"); 
    Serial.print("    Address: ");
    printAddress(THambnt);
    Serial.println();
    sensors.setResolution(THambnt, __RESOLUTION );
    Serial.print("    Resolution: ");
    Serial.print(sensors.getResolution(THambnt), DEC); 
    Serial.print("bit");
    Serial.println();
    //  ---------
    Serial.println("  Video thermal sensor");
    Serial.print("    parasite power is: ");
    if (sensors.isParasitePowerMode())
      Serial.println("ON");
    else
      Serial.println("OFF");
    if (!sensors.getAddress(THvideo, TH_VIDEO))
      Serial.println("Unable to find address for THvideo"); 
    Serial.print("    Address: ");
    printAddress(THvideo);
    Serial.println();
    sensors.setResolution(THvideo, __RESOLUTION );
    Serial.print("    Resolution: ");
    Serial.print(sensors.getResolution(THvideo), DEC); 
    Serial.print("bit");
    Serial.println();
    //  ---------
    Serial.println("  Environment thermal sensor");
    Serial.print("    parasite power is: "); 
    if (sensors.isParasitePowerMode())
      Serial.println("ON");
    else
      Serial.println("OFF");
    if (!sensors.getAddress(THenvrm, TH_ENVRM))
      Serial.println("Unable to find address for THenvrm"); 
    Serial.print("    Address: ");
    printAddress(THenvrm);
    Serial.println();
    sensors.setResolution(THenvrm, __RESOLUTION );
    Serial.print("    Resolution: ");
    Serial.print(sensors.getResolution(THvideo), DEC); 
    Serial.print("bit");
    Serial.println();
    sensors.requestTemperatures();
    aTHV = sensors.getTempC( THvideo );
    aTHE = sensors.getTempC( THenvrm );
    aTHA = sensors.getTempC( THambnt );
  } else {
    Serial.println();
    Serial.print( "Sensors found lower than the need: " );
    Serial.println( sensors.getDeviceCount() );
    bigFail();
  }

  //  ----------------------------------------------------------------- GENERAL BEGIN
  cycles = 0;
  
  Serial.println();
  Serial.println("---------------------------------------------------");
  showHelp();
  delay(5000);
  Serial.println( "" );
}



//  --------------------------------------------------------------------- LOOP
void loop(void) {
  unsigned long tTexe = millis();
  
  checkSer();
  avg_reading();
  Thrm_controller();
  LedCntr();
  SerDebug();
  SensErrorCheck();
  
  //  ------------------------------------------------------------------- Delay
  cycles = millis() / 1000;
  time( cycles );
  int dly = RFC_DELAY - ( millis() - tTexe );
  if ( dly < 0 ) dly = 1;
  delay( dly );
  Texe = ( millis() - tTexe );
  nrun++;
}
