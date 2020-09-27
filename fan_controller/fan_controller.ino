/*
 *  Reflection Fan Controller 
 *    !V 2.x
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

#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>

#include "Relay.h"
#include "ArrbotMonitor.h"

FASTLED_USING_NAMESPACE

#define RFC_VERSION         "1.9-031"

//  ------------------------------------------------------------------- GENERAL DEFINES
#define __OFF       0
#define __ON        1
#define __FOFF      -1
#define __RESET     -1

//void(* Riavvia)(void) = 0;

#define __INTERNAL    0
#define __TTTERM      1
#define __PLOT        2

#define __TERMINAL    __TTTERM

#if ( __TERMINAL == __INTERNAL )
  byte __SCROLL = __ON;
#else
  byte __SCROLL = __OFF;
#endif

#define __NORMAL      if ( __TERMINAL == __TTTERM ) Serial.print( "\e[0m" )
#define __BOLD        if ( __TERMINAL == __TTTERM ) Serial.print( "\e[1m" )

//  ------------------------------------------------------------------- RFC DEFINES
#define THE_ACTIVATION      ( aTHA + 12.0 )
#define THE_DEACTIVATION    ( aTHA + 7.0 )
#define THV_ACTIVATION      45.0
#define THV_DEACTIVATION    40.0
#define THC_ACTIVATION      42.0
#define THC_DEACTIVATION    40.0

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
int  REnable = 0;             //  Canale Main Side
int  FEnable = 0;             //  Canale Other Side
int  NEnable = 0;             //  Canale N

int   inHelp = 0;

int Controller = __ON;

byte delta = __OFF;


//  ------------------------------------------------------------------- ANALOG SENSOR DEFINES

#define SENSOR_PIN              A1
#define REFERENCE_RESISTANCE    10000
#define NOMINAL_RESISTANCE      10000
#define NOMINAL_TEMPERATURE     25.5
#define B_VALUE                 3950
#define SMOOTHING_FACTOR        10

Thermistor* originThermistor = new NTC_Thermistor( SENSOR_PIN, REFERENCE_RESISTANCE, NOMINAL_RESISTANCE, NOMINAL_TEMPERATURE, B_VALUE );
Thermistor* cpu = new SmoothThermistor( originThermistor , SMOOTHING_FACTOR );

//  ------------------------------------------------------------------- DIGITAL SENSOR DEFINES
#define ONE_WIRE_BUS  18
#define SERIAL_DIGIT  2

#define _NUM_SENSORS  3

#define __OLD         1
#define __NEW         2

#define __READING     __NEW

#if ( __READING == __OLD )
  #define __RESOLUTION  9
  #define __AVG_READ    4
#elif ( __READING == __NEW )
  #define __RESOLUTION  11
#endif

#if ( __TERMINAL == __PLOT )
  #define RFC_DELAY     1000
#else
  #define RFC_DELAY     500
#endif

#define __ERROR_CHECK   __ENABLE

#define TH_AVERAGE  20
#define DRV_AVERAGE 20

#define DRV_TH      5

#define TH_VIDEO    1
#define TH_ENVRM    2
#define TH_AMBNT    0

OneWire             oneWire(ONE_WIRE_BUS);
DallasTemperature   sensors(&oneWire);

DeviceAddress       THvideo;
DeviceAddress       THenvrm;
DeviceAddress       THambnt;

int id = 0;
int iddrv = 0;

#define             __AMB_AVERAGED        __YES
#define             __AMB_TIME  100
unsigned long       time_amb = __RESET;

int Dval = 0;
int c_Dval = 0;

float THA_s[ TH_AVERAGE ];
float THV_s[ TH_AVERAGE ];
float THE_s[ TH_AVERAGE ];
float THDrv_s[ DRV_AVERAGE ];

float aTHA = 0.0;
float aTHE = 0.0;
float aTHV = 0.0;
float aTHC = 0.0;

float aTHDrv = 0.0;

float oTHE = 0.0;
float oTHV = 0.0;
float oTHC = 0.0;

float mTHC = 0.0;
float mTHV = 0.0;
float mTHE = 0.0;

    //  -------------------------------------------- LED REFERENCE VALUE

#define THA_MAX     40.0
#define THA_MIN     20.0

#define THE_MAX     ( aTHA + 20.0 )
#define THE_MIN     aTHA

#define THV_MAX     55.0
#define THV_MIN     30.0

#define THC_MAX     50.0
#define THC_MIN     25.0

#define __EQUAL_LIMIT   600    //  uguali tra i vari sensori                   600 = 5 minuto
#define __SAME_LIMIT    1200   //  stessa lettura sul singolo sensore         1200 = 10 minuti

byte equal = 0;
byte gsame = 0;
float asame = 0;
byte esame = 0;
byte vsame = 0;
byte csame = 0;

void bigFail( void );

void  SensErrorCheck( void ) {
  byte seerror = __OFF;
  static float  otha = 0.0;
  static float  othe = 0.0;
  static float  othv = 0.0;
  static float  othc = 0.0;


  if ( otha == aTHA )
    asame+= 0.1;
  else
    asame = 0.0;
  if ( othe == aTHE )
    esame++;
  else
    esame = 0;
  if ( othv == aTHV )
    vsame++;
  else
    vsame = 0;
  if ( othc == aTHC )
    csame++;
  else
    csame = 0;
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
  othc = aTHC;
      
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

#define __VIDEO     5
#define __REAR 7
#define __FRONT 4
#define __NOT_USED  6

#define __NORMALY_OPENED  false
#define __NORMALY_CLOSED  true

Relay   RVideo     ( __VIDEO     , __NORMALY_OPENED );
Relay   RRear ( __REAR , __NORMALY_OPENED );
Relay   RFront( __FRONT , __NORMALY_OPENED );

byte Rvd = __OFF;
byte Rrr = __OFF;
byte Rfr = __OFF;
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
#define BRIGHTNESS  48

CRGB leds[NUM_LEDS];

#define _LN_SV      1
#define _LN_SE      2
#define _LN_SA      3
#define _LN_SC      0
#define _LN_PLS     4
#define _LN_CNTR    4
#define _LN_FV      6
#define _LN_FF      5
#define _LN_FT      7
#define _LN_DRV     8
#define _LN_ON      9

#define _L_0XFF     64

#define _L_ERROR_ON(a)    leds[ a ].setRGB( _L_0XFF, 0, 0 )
#define _L_ERROR_OFF(a)   leds[ a ].setRGB( 0, 0, 0 )

#define _L_ON_ON          leds[_LN_ON].setRGB( 0, _L_0XFF, 0)
#define _L_ON_OFF         leds[_LN_ON].setRGB( 0, 0, 0)

#define _L_CNTR_OFF       leds[_LN_CNTR].setRGB( 0, 0, 0)
#define _L_CNTR_INIT      leds[_LN_CNTR].setRGB( _L_0XFF, _L_0XFF, 0)
#define _L_CNTR_MANU      leds[_LN_CNTR].setRGB( _L_0XFF, 0, 0)
#define _L_CNTR_AUTO      leds[_LN_CNTR].setRGB( 0, _L_0XFF, 0)

#define _L_FT_ON          leds[_LN_FT].setRGB( 0, _L_0XFF, 0)
#define _L_FT_OFF         leds[_LN_FT].setRGB( 0, 0, 0)
#define _L_FT_DIS         leds[_LN_FT].setRGB( _L_0XFF, 0, 0)

#define _L_FF_ON          leds[_LN_FF].setRGB( 0, _L_0XFF, 0)
#define _L_FF_OFF         leds[_LN_FF].setRGB( 0, 0, 0)
#define _L_FF_DIS         leds[_LN_FF].setRGB( _L_0XFF, 0, 0)

#define _L_FV_ON          leds[_LN_FV].setRGB( 0, _L_0XFF, 0)
#define _L_FV_OFF         leds[_LN_FV].setRGB( 0, 0, 0)
#define _L_FV_DIS         leds[_LN_FV].setRGB( _L_0XFF, 0, 0)

#define _L_SA_OFF         leds[_LN_SA].setRGB( 0, 0, 0)
#define _L_SA_SET(a,b,c)  leds[_LN_SA].setRGB( a, b, c)

#define _L_SE_OFF         leds[_LN_SE].setRGB( 0, 0, 0)
#define _L_SE_SET(a,b,c)  leds[_LN_SE].setRGB( a, b, c)

#define _L_SV_OFF         leds[_LN_SV].setRGB( 0, 0, 0)
#define _L_SV_SET(a,b,c)  leds[_LN_SV].setRGB( a, b, c)

#define _L_PLS_ON         leds[_LN_PLS].setRGB( 0, 0, 128)
#define _L_PLS_OFF        leds[_LN_PLS].setRGB( 0, 0, 0)

#define _L_DRV_OFF        leds[_LN_DRV].setRGB( 0, 0, 0)
#define _L_DRV_HOT(a)     leds[_LN_DRV].setRGB( a, 0, 0)
#define _L_DRV_CLD(a)     leds[_LN_DRV].setRGB( 0, 0, a)

#define _L_NV8_OFF        leds[_LN_DRV].setRGB( 0, 0, 0)
#define _L_NS3_OFF        leds[_LN_SC].setRGB( 0, 0, 0)

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
    if ( REnable == __ENABLE )  
      Serial.print( Rrr?"M":"-" );
    else 
      Serial.print( "d" );  
    if ( FEnable == __ENABLE )
      Serial.print( Rfr?"O":"-" );
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
    if ( REnable == __ENABLE )  
      Serial.print( Rrr?"\e[1mM\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );  
    if ( FEnable == __ENABLE )
      Serial.print( Rfr?"\e[1mO\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );  
    if ( NEnable == __ENABLE )
      Serial.print( Rnu?"\e[1mN\e[0m":"-" );
    else 
      Serial.print( "\e[1md\e[0m" );
  }
}

//  ------------------------------------------------------------------- CODE START

void avg_reset( void ) {
  for( int c = 0 ; c < TH_AVERAGE ; c++ )
    THA_s[c] = aTHA;
  for( int c = 0 ; c < TH_AVERAGE ; c++ )
    THE_s[c] = aTHE;
  for( int c = 0 ; c < TH_AVERAGE ; c++ )
    THV_s[c] = aTHV;
  for( int c = 0 ; c < DRV_AVERAGE ; c++ )
    THDrv_s[c] = 0.0;

  aTHDrv = 0.0;
}


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
  if ( __TERMINAL != __PLOT ) {
    if ( delta == __OFF ) {
      if ( Texe ) {
        if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
          Serial.println("\e[1K");
          Serial.print("\e[1A");
        }
        Serial.print(" CH A/E/V/C: ");
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
        Serial.print("/");
        __BOLD;
        Serial.print(aTHC , SERIAL_DIGIT );
        __NORMAL;
        Serial.print(" - Der_v: ");
        __BOLD;
        Serial.print( aTHDrv * 1000 , SERIAL_DIGIT );
        __NORMAL;
        Serial.print( "/" );
        Serial.print( Dval );
        Serial.print( "/" );
        Serial.print( c_Dval );
        __NORMAL;
        Serial.print(" - Me/Mv/Mc: ");
        __BOLD;
        Serial.print(mTHE , SERIAL_DIGIT );
        __NORMAL;
        Serial.print("/");
        __BOLD;
        Serial.print(mTHV , SERIAL_DIGIT );
        __NORMAL;
        Serial.print("/");
        __BOLD;
        Serial.print(mTHC , SERIAL_DIGIT );
        __NORMAL;
        Serial.print("    -  ");
        printStatus();
        Serial.print( abs( Texe - RFC_DELAY ) < 2?" OK ":" NOT OK " );
      }
    }
    else if ( delta == __ON ) {
      if ( Texe ) {
        if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
          Serial.println("\e[1K");
          Serial.print("\e[1A");
        }
        Serial.print(" CH A/dE/dV/dC: ");
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
        Serial.print("/");
        __BOLD;
        Serial.print(aTHC - aTHA , SERIAL_DIGIT );
        __NORMAL;
        Serial.print(" - Eq/Sm-a-e-v-c: ");
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
        Serial.print("-");
        Serial.print(csame);
        __NORMAL;
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
  else {
    DISPLAY( RFC_VERSION );

    MONITOR2( "Room" , aTHA );
    MONITOR2( "Case" , aTHE );
    MONITOR2( "Video" , aTHV );
    MONITOR2( "Cpu" , aTHC );

    DISPLAY2( "Room" , aTHA );
    DISPLAY2( "Case" , aTHE );
    DISPLAY2( "Video" , aTHV );
    DISPLAY2( "Cpu" , aTHC );
    DISPLAY2( "Derivative" , aTHDrv * 1000 );

    MONITOR_ENDL();
  }
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
          RVideo.turnOn();
        } else {
          RVideo.turnOff();
        }
        mod = 1;
      }
      break;
    }
    case __REAR : {
      if ( Rrr != st ) {
        Rrr = st;
        if ( st == __ON ) {
          RRear.turnOn();
          _L_FF_ON;
        } else {
          RRear.turnOff();
          _L_FF_OFF;
        }
        mod = 1;
      }
      break;
    }
    case __FRONT : {
      if ( Rfr != st ) {
        Rfr = st;
        if ( st == __ON ) {
          RFront.turnOn();
          _L_FT_ON;
        } else {
          RFront.turnOff();
          _L_FT_OFF;
        }
        mod = 1;
      }
      break;
    }
/*    case __NOT_USED : {
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
*/
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
    if ( aTHC > mTHC ) mTHC = aTHC;
    if ( aTHV > mTHV ) mTHV = aTHV;
    if ( aTHE > mTHE ) mTHE = aTHE;
  }

#elif ( __READING == __NEW ) 

  void avg_reading( void ) {
    oTHV = aTHV;
    sensors.requestTemperatures();
    THV_s[ id ] = sensors.getTempC( THvideo );
    THE_s[ id ] = sensors.getTempC( THenvrm );
    #if ( __AMB_AVERAGED == __YES )
      THA_s[ id ] = sensors.getTempC( THambnt );
    #else
      aTHA = sensors.getTempC( THambnt );
    #endif      
    for( int a = 0; a < TH_AVERAGE ; a++ ) {
      aTHV += THV_s[a];
      aTHE += THE_s[a];
      #if ( __AMB_AVERAGED == __YES )
        aTHA += THA_s[a];
      #endif
    }
    #if ( __AMB_AVERAGED == __YES )
      aTHA /= (float)TH_AVERAGE;
    #else
      aTHA = sensors.getTempC( THambnt );
    #endif
    aTHV /= (float)TH_AVERAGE;
    aTHE /= (float)TH_AVERAGE;

    aTHC = cpu->readCelsius();

    if ( AutoInit == __ON ) {
      THDrv_s[ iddrv ] = aTHV -oTHV;
      for( int a = 0; a < DRV_AVERAGE ; a++ ) {
        aTHDrv += THDrv_s[a];
      }
      aTHDrv /= (float)DRV_AVERAGE;
    } else {
      iddrv = 0;
      aTHDrv = 0;
    }
    
    if ( ++id >= TH_AVERAGE )
      id = 0;
    if ( ++iddrv >= DRV_AVERAGE )
      iddrv = 0;
      
    if ( aTHE > mTHE ) mTHE = aTHE;
    if ( aTHV > mTHV ) mTHV = aTHV;
    if ( aTHC > mTHC ) mTHC = aTHC;
  }

#endif

void  Thrm_controller( void ) {
  if ( cycles < 15 ) {
    AutoInit = __OFF;
  } 
  else {
    
    AutoInit = __ON;
  }

  if ( AutoInit == __ON ) {
    if ( ! Force  ) {
      Controller = __ON;
    }
	else {
      Controller = __OFF;
    }
      
    if ( Controller == __ON ) {
      Relay_manager( __NOT_USED , __OFF );
/*
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
        if ( REnable == __ENABLE ) Relay_manager( __REAR , __ON );
        if ( FEnable == __ENABLE ) Relay_manager( __FRONT , __ON );
      }
      else if ( aTHV > ( ( THV_DEACTIVATION + THV_ACTIVATION ) / 2 ) ) {
        if ( REnable == __ENABLE ) Relay_manager( __REAR , __ON );
      }
      else {
        if ( ( aTHE < THE_ACTIVATION ) && ( aTHV < THV_ACTIVATION ) ) {
          if ( FEnable == __ENABLE ) Relay_manager( __FRONT , __OFF );
        }
        if ( aTHE < THE_DEACTIVATION ) {
          if ( REnable == __ENABLE ) Relay_manager( __REAR , __OFF );
        }
      }
 */
	//  ----------------------------------------  TH Environment
		if ( aTHE > THE_ACTIVATION ) {
			if ( FEnable == __ENABLE ) Relay_manager( __FRONT , __ON );
		}
		else if ( aTHE < THE_DEACTIVATION ) {
			if ( FEnable == __ENABLE ) Relay_manager( __FRONT , __OFF );
		}
		
	//  ----------------------------------------  TH Video
		if ( aTHV > THV_ACTIVATION ) {
			if ( VEnable == __ENABLE ) Relay_manager( __VIDEO , __ON );
		}
		else if ( aTHV < THV_DEACTIVATION ) {
			if ( FEnable == __ENABLE ) Relay_manager( __VIDEO , __OFF );
		}
		
	//  ----------------------------------------  TH CPU
		if ( aTHC > THC_ACTIVATION ) {
			if ( REnable == __ENABLE ) Relay_manager( __REAR , __ON );
		}
		else if ( aTHC < THC_DEACTIVATION ) {
			if ( REnable == __ENABLE ) Relay_manager( __REAR , __OFF );
		}
	}
    else {
      if ( Force ) {
        Relay_manager( __VIDEO , __ON );
        Relay_manager( __REAR , __ON );
        Relay_manager( __FRONT , __ON );
        Relay_manager( __NOT_USED , __ON );
      }
      if ( SForce ) {
        Relay_manager( __VIDEO , __OFF );
        Relay_manager( __REAR , __OFF );
        Relay_manager( __FRONT , __OFF );
        Relay_manager( __NOT_USED , __OFF );
      }
    }
  } /* else {
    Force = 1;
    Relay_manager( __VIDEO , __ON );
    Relay_manager( __REAR , __ON );
    Relay_manager( __FRONT , __ON );
    Relay_manager( __NOT_USED , __ON );
  } */
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
        REnable = 0;
        FEnable = 0;
        NEnable = 0;
        SForce = 0;

        break;
      }
      case 'f': {
        VEnable = __DISABLE;
        Relay_manager( __VIDEO , __OFF );
        REnable = __DISABLE;
        Relay_manager( __REAR , __OFF );
        FEnable = __DISABLE;
        Relay_manager( __FRONT , __OFF );

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
        if ( inByte == 'M' ) REnable = __ENABLE;
        if ( inByte == 'm' ) {
          Relay_manager( __REAR , __OFF );
          REnable = __DISABLE;
        }
        break;
      }
      case 'O':
      case 'o': {
        if ( inByte == 'O' ) FEnable = __ENABLE;
        if ( inByte == 'o' ) {
          Relay_manager( __FRONT , __OFF );
          FEnable = __DISABLE;
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
  _L_PLS_OFF;  
  _L_UPDATE;
  delay( 20 );
  _L_PLS_ON;
  _L_UPDATE;
  delay( 60 );
  _L_PLS_OFF;  
  _L_UPDATE;
  delay( 20 );
  
  if ( VEnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_FV_DIS;
      else
        _L_FV_OFF;
  } else {
    if ( Rvd == __OFF ) {
      _L_FV_OFF;
    } else {
      _L_FV_ON;
    }
  }
  
  if ( REnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_FF_DIS;
      else
        _L_FF_OFF;
  } else {
    if ( Rrr == __OFF ) {
      _L_FF_OFF;
    } else {
      _L_FF_ON;
    }
  }

  if ( FEnable == __DISABLE ) {
    if ( nrun % 2 )
        _L_FT_DIS;
      else
        _L_FT_OFF;
  } else {
    if ( Rfr == __OFF ) {
      _L_FT_OFF;
    } else {
      _L_FT_ON;
    }
  }

  if ( Controller == __ON )
    _L_CNTR_AUTO;
  else {
    if ( nrun % 2 ) {
      _L_CNTR_MANU;
      _L_FF_ON;
      _L_FT_ON;
      _L_FV_ON;
    } else {
      _L_CNTR_OFF;
      _L_FF_OFF;
      _L_FT_OFF;
      _L_FV_OFF;
    }
  }

  if ( AutoInit == __ON ) {
    set_ls_temp( _LN_SA , aTHA );
    set_ls_temp( _LN_SE , aTHE );
    set_ls_temp( _LN_SV , aTHV );
    set_ls_temp( _LN_SC , aTHC );
    
  } else {
    if ( nrun % 2 )
        _L_CNTR_INIT;
      else
        _L_CNTR_OFF;
   }

  if ( AutoInit == __ON ) {
    c_Dval = constrain( abs( aTHDrv  * 1000 ) , 0 , 60 );
    Dval = map( c_Dval , 0 , 60 , 0 , 255 );
    if ( c_Dval < 3 ) {
      _L_DRV_OFF;
    } else {
      if ( aTHDrv > 0 ) {
        _L_DRV_HOT( Dval );
      } else {
        _L_DRV_CLD( Dval );
      }
    }
  }
  
  if ( gsame > ( __SAME_LIMIT * 0.2 ) )
    _L_CNTR_INIT;

  _L_UPDATE;
}

//  ------------------------------------------------------------------- SETUP
void setup(void) {

  delay( 1000 );

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
 
  _ALL_LED_OFF
//  _L_ON_ON;
  _L_UPDATE;
  
  for( byte a = 0; a < TH_AVERAGE ; a++ ) 
    THV_s[a] = 0.0;
  for( byte a = 0; a < TH_AVERAGE ; a++ )
    THE_s[a] = 0.0;

  Serial.begin(115200);

  if ( __TERMINAL != __PLOT ) {
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
  }
  else
    MONITOR_RESET();

  
  RVideo.begin();
  RRear.begin();
  RFront.begin();

  delay( 1000 );

  //  ------------------------------------------------------------------- SENSORE BEGIN
  pinMode( ONE_WIRE_BUS , INPUT_PULLUP );
  if ( __TERMINAL != __PLOT ) {
    Serial.println();
    Serial.print("Thermal Sensor Initializing [DS18B20]...");
    Serial.println();
    Serial.print("  Locating devices...");
  }
  sensors.begin();
  if ( __TERMINAL != __PLOT ) {
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
    Serial.println();
  }
  else
    sensors.getDeviceCount();
  if ( sensors.getDeviceCount() == _NUM_SENSORS ) {
    //  ---------
    if ( __TERMINAL != __PLOT ) {
      Serial.println("  Ambient thermal sensor");
      Serial.print("    parasite power is: ");
      if ( sensors.isParasitePowerMode() )
        Serial.println("ON");
      else
        Serial.println("OFF");
      if (!sensors.getAddress(THambnt, TH_AMBNT))
        Serial.println("Unable to find address for THambnt"); 
      Serial.print("    Address: ");
      printAddress(THambnt);
      Serial.println();
    }
    else {
      sensors.isParasitePowerMode();
      sensors.getAddress(THambnt, TH_AMBNT);
    }
    sensors.setResolution(THambnt, __RESOLUTION );
    if ( __TERMINAL != __PLOT ) {
      Serial.print("    Resolution: ");
      Serial.print(sensors.getResolution(THambnt), DEC); 
      Serial.print(" bit");
      Serial.println();
    }
    else
      sensors.getResolution(THambnt);
    sensors.getDeviceCount();
    _L_SET( 7 , _L_0XFF , _L_0XFF , 0 );
    _L_UPDATE;
    delay( 500 );

    //  ---------
    if ( __TERMINAL != __PLOT ) {
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
    }
    else {
      sensors.isParasitePowerMode();
      sensors.getAddress(THvideo, TH_VIDEO);
    }
    sensors.setResolution(THvideo, __RESOLUTION );
    if ( __TERMINAL != __PLOT ) {
      Serial.print("    Resolution: ");
      Serial.print(sensors.getResolution(THvideo), DEC); 
      Serial.print(" bit");
      Serial.println();
    }
    else
      sensors.getResolution(THvideo);
    _L_SET( 6 , _L_0XFF , _L_0XFF , 0 );
    _L_UPDATE;
    delay( 500 );

    //  ---------
    if ( __TERMINAL != __PLOT ) {
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
    }
    else {
      sensors.isParasitePowerMode();
      sensors.getAddress(THenvrm, TH_ENVRM);
    }
    sensors.setResolution(THenvrm, __RESOLUTION );
    if ( __TERMINAL != __PLOT ) {
      Serial.print("    Resolution: ");
      Serial.print(sensors.getResolution(THvideo), DEC); 
      Serial.print(" bit");
      Serial.println();
    }
    else
      sensors.getResolution(THenvrm);
    _L_SET( 5 , _L_0XFF , _L_0XFF , 0 );
    _L_UPDATE;
    delay( 3000 );
    _L_ON_ON;
    _L_UPDATE;

    //  ---------
    sensors.requestTemperatures();
    aTHV = sensors.getTempC( THvideo );
    aTHE = sensors.getTempC( THenvrm );
    aTHA = sensors.getTempC( THambnt );
    aTHC = cpu->readCelsius();
    avg_reset();
  } else {
    if ( __TERMINAL != __PLOT ) {
      Serial.println();
      Serial.print( "Sensors found lower than the need: " );
      Serial.println( sensors.getDeviceCount() );
    }
    for ( int i = 0 ; i < sensors.getDeviceCount() ; i++ ) {
      _L_SET( ( 8 - i ) , _L_0XFF , _L_0XFF , 0 );
      _L_UPDATE;
      delay( 500 );
    }
    _L_ON_ON;
    _L_UPDATE;
    delay( 3000 );
    bigFail();
  }

  //  ----------------------------------------------------------------- GENERAL BEGIN
  cycles = 0;
  
  if ( __TERMINAL != __PLOT ) {
    Serial.println();
    Serial.println("---------------------------------------------------");
  }
  if ( __TERMINAL != __PLOT )
    showHelp();
  _L_SET( 7 , 0 , 0 , 0 );
  _L_SET( 6 , 0 , 0 , 0 );
  _L_SET( 5 , 0 , 0 , 0 );
  _L_UPDATE;
  delay(2000);
  if ( __TERMINAL != __PLOT ) {
    Serial.println( "" );
  }
}



//  --------------------------------------------------------------------- LOOP
void loop(void) {
  unsigned long tTexe = millis();
  
  checkSer();
  avg_reading();
//  if ( cycles < 3 )
//    avg_reset();
  Thrm_controller();
  LedCntr();
  SerDebug();
  if ( __ERROR_CHECK == __ENABLE )
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
