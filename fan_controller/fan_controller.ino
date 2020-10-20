/*
 *  Reflection Fan contr 
 *    !V 2.x
 *    !GPL V3
 *    
 *  gKript.org @ 09/2020 (R)
 *    !asyntote
 *    !skymatrix
 *    
 *  @Arduino Due
 */

/*
- Posizione del cursore:
    \033[<L>;<C>H
      mette il cursore alla linea L e colonna C.
- Muove il cursore su N linee:
    \033[<N>A
- Muove il cursore giù N linee:
    \033[<N>B
- Muove il cursore avanti N colonne:
    \033[<N>C
- Muove il cursore indietro N colonne:
    \033[<N>D
*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastLED.h>

#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>
#include <AverageThermistor.h>

#include "Relay.h"
#include "ArrbotMonitor.h"

FASTLED_USING_NAMESPACE

#define RFC_VERSION         "1.10-193"

//  ------------------------------------------------------------------- GENERAL DEFINES
#define __OFF       0
#define __ON        1
#define __FOFF      -1
#define __RESET     -1

#define __INTERNAL    0
#define __TTTERM      1
#define __PLOT        2
#define __MONITOR     3

#define __TEMP        10
#define __DRVS        11
#define __LINE        12
#define __TABLE       13

#define __TERMINAL    __TTTERM

#define __CONTENT     __TABLE


#if ( __TERMINAL == __INTERNAL )
  # undef __CONTENT
  #define __CONTENT     __LINE
  byte __SCROLL = __ON;
#elif ( __TERMINAL == __MONITOR )
  # undef __CONTENT
  #define __CONTENT     __TABLE
  byte __SCROLL = __OFF;
#else
  byte __SCROLL = __OFF;
#endif

#define __BLACK           30
#define __RED             31
#define __GREEN           32
#define __YELLOW          33
#define __BLUE            34
#define __MAGENA          35
#define __CYAN            36
#define __WHITE           37
#define __GRAY            90
#define __BRIGHT_RED      91
#define __BRIGHT_GREEN    92
#define __BRIGHT_YELLOW   93
#define __BRIGHT_BLUE     94
#define __BRIGHT_MAGENTA  95
#define __BRIGHT_CYAN     96
#define __BRIGHT_WHITE    97

#define __NORMAL                  if ( __TERMINAL == __TTTERM ) Serial.print( "\e[0m" )
#define __BOLD                    if ( __TERMINAL == __TTTERM ) Serial.print( "\e[1m" )
#define __BLANK                   Serial.print( " " )

#define __SERPRINT( a )           Serial.print( a )
#define __SERPRINTLN( a )         Serial.println( a )
#define __SERPRINT2( a , b )      Serial.print( a , b )
#define __SERPRINT2LN( a , b )    Serial.println( a , b )
#define __SERLN                   Serial.println()

void __escape_code_comp( unsigned char code ) {
  Serial.print( "\e[" );
  Serial.print( code );
  Serial.print( "m" );
}
                                
#define __SERBOLD( a )            if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[1m" );    \
                                  Serial.print( a );            \
                                  if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[0m" );    \
                                
#define __SERBOLD2( a , b )       if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[1m" );    \
                                  Serial.print( a , b );        \
                                  if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[0m" );    \

#define __SERCOLOR( a, col )      if ( __TERMINAL == __TTTERM ) \
                                    __escape_code_comp( col );  \
                                  Serial.print( a );            \
                                  if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[0m" );    \
                                
#define __SERCOLOR2( a , b, col ) if ( __TERMINAL == __TTTERM ) \
                                    __escape_code_comp( col );  \
                                  Serial.print( a , b );        \
                                  if ( __TERMINAL == __TTTERM ) \
                                    Serial.print( "\e[0m" );    \

// Serial.print BLink if (a) is Higher than (b) then (c)
#define __SERBLC( a, c )           if ( 1 ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERCOLOR( a,  c );  \
                                    }                   \
                                    else {              \
                                      __SERPRINT( a );   \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Higher than (b) then (c)
#define __SERBLC2( a, b, c )      if ( 1 ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERCOLOR2( a, b, c );  \
                                    }                   \
                                    else {              \
                                      __SERPRINT2( a, b );   \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Higher than (b) then (c)
#define __SERBLH( a, b, c )       if ( a > b ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERBOLD( c );   \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Lower than (b) then (c)
#define __SERBLL( a, b, c)        if ( a < b ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERBOLD( c );   \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Equal to (b) then (c)
#define __SERBLE( a, b, c)        if ( a ==  b ) {      \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERBOLD( c );   \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Higher than (b) then (c)
#define __SERBLCH( a, b, c, d)   if ( a > b ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERCOLOR( c, d ); \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \


// Serial.print BLink if (a) is Lower than (b) then (c)
#define __SERBLCL( a, b, c, d)    if ( a < b ) {        \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERCOLOR( c, d ); \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Equal to (b) then (c)
#define __SERBLCE( a, b, c, d)    if ( a ==  b ) {      \
                                    static char tg = 0; \
                                    if ( tg ) {         \
                                      __SERPRINT( c );  \
                                    }                   \
                                    else {              \
                                      __SERCOLOR( c, d ); \
                                    }                   \
                                    tg ^= 1;            \
                                    __BLANK;            \
                                  }                     \

// Serial.print BLink if (a) is Higher than (b) then (c)
#define __SERBLCH2( a, b, c, d, e)  if ( a > b ) {        \
                                      static char tg = 0; \
                                      if ( tg ) {         \
                                        __SERPRINT2( c, d );   \
                                      }                   \
                                      else {              \
                                        __SERCOLOR2( c, d, e); \
                                      }                   \
                                      tg ^= 1;            \
                                      __BLANK;            \
                                    }                     \

// Serial.print BLink if (a) is Lower than (b) then (c)
#define __SERBLCL2( a, b, c, d, e)  if ( a < b ) {        \
                                      static char tg = 0; \
                                      if ( tg ) {         \
                                        __SERPRINT2( c, d );   \
                                      }                   \
                                      else {              \
                                        __SERCOLOR2( c, d, e); \
                                      }                   \
                                      tg ^= 1;            \
                                      __BLANK;            \
                                    }                     \

// Serial.print BLink if (a) is Equal to (b) then (c)
#define __SERBLCE2( a, b, c, d, e)  if ( a ==  b ) {      \
                                      static char tg = 0; \
                                      if ( tg ) {         \
                                        __SERPRINT2( c, d );   \
                                      }                   \
                                      else {              \
                                        __SERCOLOR2( c, d, e); \
                                      }                   \
                                      tg ^= 1;            \
                                      __BLANK;            \
                                    }                     \

// Serial.print Bold if (a) is Higher than (b) then (c)
#define __SERBH( a, b, c)           if ( a > b ) {    \
                                      __SERBOLD( c ); \
                                      __BLANK;        \
                                    }                 \

// Serial.print Bold if (a) is Lower than (b) then (c)
#define __SERBL( a, b, c)           if ( a < b ) {    \
                                      __SERBOLD( c ); \
                                      __BLANK;        \
                                    }                 \

// Serial.print Bold if (a) is Equal to (b) then (c)
#define __SERBE( a, b, c)           if ( a == b ) {   \
                                      __SERBOLD( c ); \
                                      __BLANK;        \
                                    }                 \

// Serial.print COLOR if (a) is Higher than (b) then (c) in color (d)
#define __SERCH( a, b, c, d)        if ( a > b ) {        \
                                      __SERCOLOR( c, d ); \
                                      __BLANK;            \
                                    }                     \

// Serial.print COLOR if (a) is Lower than (b) then (c) in color (d)
#define __SERCL( a, b, c, d)        if ( a < b ) {        \
                                      __SERCOLOR( c, d ); \
                                      __BLANK;            \
                                    }                     \

// Serial.print COLOR if (a) is Equal to (b) then (c) in color (d)
#define __SERCE( a, b, c, d)        if ( a == b ) {       \
                                      __SERCOLOR( c, d ); \
                                      __BLANK;            \
                                    }                     \

// Serial.print COLOR if (a) is Higher than (b) then (c) in color (d)
#define __SERCH2( a, b, c, d, e)    if ( a > b ) {           \
                                      __SERCOLOR2( c, d, e); \
                                      __BLANK;               \
                                    }                        \

// Serial.print COLOR if (a) is Lower than (b) then (c) in color (d)
#define __SERCL2( a, b, c, d, e)    if ( a < b ) {           \
                                      __SERCOLOR2( c, d, e); \
                                      __BLANK;               \
                                    }                        \

// Serial.print COLOR if (a) is Equal to (b) then (c) in color (d)
#define __SERCE2( a, b, c, d, e)    if ( a == b ) {          \
                                      __SERCOLOR2( c, d, e); \
                                      __BLANK;               \
                                    }                        \

//  ------------------------------------------------------------------- RFC DEFINES
#define THE_ACTIVATION      ( aTHA + 15.0 )
#define THE_DEACTIVATION    ( aTHA + 8.0 )
#define THV_ACTIVATION      ( aTHA + 28.0 ) // 45.0
#define THV_DEACTIVATION    ( aTHA + 18.0 ) // 40.0
#define THC_ACTIVATION      ( aTHA + 23.0 ) // 42.0
#define THC_DEACTIVATION    ( aTHA + 16.0 ) // 40.0

#define __ERROR_STOP        while(1)

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

int contr = __ON;

byte delta = __OFF;

//  ------------------------------------------------------------------- DIGITAL SENSOR DEFINES
#define ONE_WIRE_BUS  18
#define SERIAL_DIGIT  1

unsigned char Tdigit = SERIAL_DIGIT;

#define _NUM_SENSORS  3

#define __OLD         1
#define __NEW         2

#if ( __TERMINAL == __PLOT )
  #define __RESOLUTION  12
#else
  #define __RESOLUTION  11
#endif

#if ( __TERMINAL == __PLOT )
  #define RFC_DELAY     1000
#else
  #define RFC_DELAY     500
#endif

#define __ERROR_CHECK   __ENABLE

#define __THERMAL_SMOOTH_FACTOR       50
#define __DERIVATIVE_SMOOTH_FACTOR    30

#if ( __TERMINAL == __PLOT )
  #define TH_AVERAGE        __THERMAL_SMOOTH_FACTOR
  #define TH_ANAL_AVERAGE   __THERMAL_SMOOTH_FACTOR
  #define DRV_AVERAGE       __DERIVATIVE_SMOOTH_FACTOR
#else 
  #define TH_AVERAGE        ( __THERMAL_SMOOTH_FACTOR / 2 )
  #define TH_ANAL_AVERAGE   ( __THERMAL_SMOOTH_FACTOR )
  #define DRV_AVERAGE       ( __DERIVATIVE_SMOOTH_FACTOR / 2 )
#endif

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

#define             __AMB_AVERAGED        __NO
#define             __AMB_TIME            9500

unsigned long       time_amb = __RESET;

int Dval = 0;
int c_Dval = 0;

float THA_s[ TH_AVERAGE ];
float THE_s[ TH_AVERAGE ];
float THV_s[ TH_AVERAGE ];
float THC_s[ TH_AVERAGE ];

float THEDrv_s[ DRV_AVERAGE ];
float THVDrv_s[ DRV_AVERAGE ];
float THCDrv_s[ DRV_AVERAGE ];
float THDrv_s[ DRV_AVERAGE ];

float aTHA = 0.0;
float aTHE = 0.0;
float aTHV = 0.0;
float aTHC = 0.0;

float aTHEDrv = 0.0;
float aTHVDrv = 0.0;
float aTHCDrv = 0.0;
float aTHDrv = 0.0;

float oTHE = 0.0;
float oTHV = 0.0;
float oTHC = 0.0;
float oTHDrv = 0.0;

float MTHA = 0.0;
float MTHE = 0.0;
float MTHV = 0.0;
float MTHC = 0.0;

float mTHA = 100.0;
float mTHE = 100.0;
float mTHV = 100.0;
float mTHC = 100.0;

float pTHA = 0.0;
float pTHE = 0.0;
float pTHV = 0.0;
float pTHC = 0.0;
float pTHGen = 0.0;


//  ------------------------------------------------------------------- ANALOG SENSOR DEFINES

#ifdef ARDUINO_SAM_DUE
  #define __MAX_ANALOG_RES      12
#else
  #define __MAX_ANALOG_RES      10
#endif

#define SENSOR_PIN              A1
#define REFERENCE_RESISTANCE    10000
#define NOMINAL_RESISTANCE      10000
#define NOMINAL_TEMPERATURE     25.5
#define B_VALUE                 3950
#define SMOOTHING_FACTOR        TH_ANAL_AVERAGE

Thermistor* oCPU = new NTC_Thermistor( SENSOR_PIN, REFERENCE_RESISTANCE, NOMINAL_RESISTANCE, NOMINAL_TEMPERATURE, B_VALUE );
Thermistor* cpu = new AverageThermistor( oCPU , TH_ANAL_AVERAGE , 1 );
//Thermistor* cpu  = new SmoothThermistor( oCPU , SMOOTHING_FACTOR );

//  ------------------------------------------------------------------- AIR BOMB DEFINES

#define __AIR_BOMB              __OFF

#define __INACTIVE              0
#define __CHARGING              1 
#define __IGNITION              2                           // Gestito dalla Controller
#define __RUNNING               3
#define __SWITCH_OFF            4                           // Gestito dalla Controller
#define __DISABLED              10

#define __AB_THRESHOLD          30.0
#define __AB_TIMEOUT            5                           //  Espresso in minuti

unsigned long AB_time = millis();
#if ( __AIR_BOMB ==  __ON )
  byte AB_state = __INACTIVE;
#else
  byte AB_state = __DISABLED;
#endif
  
byte AB_force = __OFF;

//  ------------------------------------------------------------------- LED REFERENCE VALUE

#define THA_MAX         40.0
#define THA_MIN         20.0

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

#define __VIDEO     4
#define __REAR      5   // 7
#define __FRONT     7   // 4
#define __NOT_USED  6

#define __NORMALY_OPENED  false
#define __NORMALY_CLOSED  true

Relay   RVideo  ( __VIDEO , __NORMALY_OPENED );
Relay   RRear   ( __REAR  , __NORMALY_OPENED );
Relay   RFront  ( __FRONT , __NORMALY_OPENED );

byte Rvd = __OFF;
byte Rrr = __OFF;
byte Rfr = __OFF;
byte Rnu = __OFF;

//  ------------------------------------------------------------------- MACROS FROM DateTime.h
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_)   (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)

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
#define _L_ON_ON_HT       leds[_LN_ON].setRGB( _L_0XFF, 0, 0)
#define _L_ON_ON_LT       leds[_LN_ON].setRGB( _L_0XFF, 0, 0)
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

#define _L_PLS_ON         leds[_LN_PLS].setRGB( 0, 0, 0)
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

void  set_ls_temp( byte led , float perc ) {
  int rv = (int)fmap( perc , 0.0 , 100.0 , 0.0 , _R_MAX );
  int bv = 255 - rv; 
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
      if ( contr == __ON ) 
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
  else if( __TERMINAL == __TTTERM ){
    if( __TERMINAL == __TTTERM ) {
      Serial.print("Control: ");
      if ( AutoInit == __OFF )
        Serial.print("INIT   ");
      else {
        if ( contr == __ON ) 
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
    THEDrv_s[c] = 0.0;
  for( int c = 0 ; c < DRV_AVERAGE ; c++ )
    THVDrv_s[c] = 0.0;
  for( int c = 0 ; c < DRV_AVERAGE ; c++ )
    THCDrv_s[c] = 0.0;
  for( int c = 0 ; c < DRV_AVERAGE ; c++ )
    THDrv_s[c] = 0.0;

  aTHDrv = 0.0;
  oTHE = aTHE;
  oTHV = aTHV;
  oTHC = aTHC;
  oTHDrv = aTHDrv;
}


void bigFail( void ) {
  byte r = 0;
  _ALL_LED_OFF;
  __ERROR_STOP {
    commandAcq();
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


void printLine( void )  {
  if ( delta == __OFF ) {
    if ( Texe ) {
      if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
        Serial.println("\e[1K");
        Serial.print("\e[1A");
      }

      Serial.print(" CH A/E/V/C: ");
      __BOLD;
      Serial.print(aTHA , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHE , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHV , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHC , Tdigit );
      __NORMAL;
      Serial.print(" - Der_v: ");
      __BOLD;
      Serial.print( aTHDrv * 1000 , Tdigit );
      __NORMAL;
      Serial.print( "/" );
      Serial.print( Dval );
      Serial.print( "/" );
      Serial.print( c_Dval );

      __NORMAL;
      Serial.print(" - Me/Mv/Mc: ");
      __BOLD;
      Serial.print(MTHE , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(MTHV , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(MTHC , Tdigit );

      __NORMAL;
      Serial.print("  -  ");
      if ( hours < 10 )
        Serial.print("Up time: 0");
      else
        Serial.print("Up time: ");
      Serial.print(hours);
      if ( minutes < 10 )
        Serial.print(":0");
      else
        Serial.print(":");
      Serial.print(minutes);
      if ( seconds < 10 )
        Serial.print(":0");
      else
        Serial.print(":");
      Serial.print(seconds);

      __NORMAL;
      Serial.print("  -  ");
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
      
      Serial.print(" A/DrvE/DrvV/DrvC/DrvTOT: ");
      __BOLD;
      Serial.print(aTHA , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHEDrv * 1000, Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHVDrv * 1000 , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHCDrv * 1000 , Tdigit );
      __NORMAL;
      Serial.print("/");
      __BOLD;
      Serial.print(aTHDrv * 1000 , Tdigit );

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
}


void printTable( void ) {
  if ( inHelp ) {
    Serial.println();
    showHelp();
    inHelp = 0;
  }

  if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) ) {
    Serial.println("\e[1K");
    Serial.print("\e[30A");
  }

  Serial.print("Proxima Fan contr - Version: " );
    Serial.print( RFC_VERSION );
    Serial.print(" - Up time: ");
    if ( hours < 10 )
      Serial.print("0");
    Serial.print(hours);
    Serial.print(":");
    if ( minutes < 10 )
      Serial.print("0");
    Serial.print(minutes);
    Serial.print(":");
    if ( seconds < 10 )
      Serial.print("0");
    Serial.print(seconds);
  Serial.println();
  Serial.println( "  !asyntote [gKript.org] 2020" );
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
  Serial.println("CONTROLLER:");
    Serial.print("\tStatus:\t\t\t");
      __SERBLCE   ( AutoInit  , __OFF , "Init  ", __BRIGHT_YELLOW )
      else {
        __SERCE   ( contr     , __ON  , "Auto  ", __BRIGHT_GREEN );
        __SERBLCE ( contr     , __OFF , "MANUAL", __BRIGHT_RED );
      }
//      Serial.print("\t");
      Serial.print("  - Percentage:\t\t");
      __SERCE2( pTHGen , 0.00 , pTHGen , Tdigit , __BRIGHT_BLUE )
      else __SERBLCH2( pTHGen , 90.0 , pTHGen , Tdigit , __BRIGHT_RED )
      else __SERBLCH2( pTHGen , 80.0 , pTHGen , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( pTHGen , Tdigit );
      }
      __SERPRINT( "%" );
      __BLANK;
    Serial.println();
    Serial.print("\tScan [");
      Serial.print( Texe );
      Serial.print( "ms]:" );
      __BLANK;
      Serial.print( "\t\t" );
      Serial.print( abs( Texe - RFC_DELAY ) < 2?"OK    ":"NOT OK" );
      Serial.print("\t");
      Serial.print(" - General Derivative:\t");
      __SERCH2( ( aTHDrv * 1000 ), 40.00, ( aTHDrv * 1000 ), Tdigit, __BRIGHT_RED )
      else __SERCH2( ( aTHDrv * 1000 ), 30.00, ( aTHDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else __SERCL2( ( aTHDrv * 1000 ), -40.00, ( aTHDrv * 1000 ), Tdigit, __BRIGHT_BLUE )
      else __SERCL2( ( aTHDrv * 1000 ), -30.00, ( aTHDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else {
        __SERCOLOR2( ( aTHDrv * 1000 ) , Tdigit, __BRIGHT_WHITE );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tAir Bomb:\t\t");
    if ( AB_state ==  __INACTIVE ) {
      __SERPRINT( "Inactive               " );
    }
    else if ( AB_state ==  __CHARGING ) {
      __SERBLC(   "Charging -", __BRIGHT_YELLOW );
      printDigits( ( ( ( __AB_TIMEOUT * 60 ) - ( ( millis() - AB_time) / 1000 ) ) / 60 ) % 60 );
      __SERPRINT( ":" );
      printDigits( ( ( __AB_TIMEOUT * 60 ) - ( ( millis() - AB_time) / 1000 ) ) % 60 );
    }
    else if ( AB_state ==  __RUNNING ) {
      __SERBLC(   "RUNNING                ", __BRIGHT_RED );
    }
    else if ( AB_state ==  __IGNITION ) {
      __SERCOLOR( "Ignition               ", __BRIGHT_YELLOW );
    }
    else if ( AB_state ==  __SWITCH_OFF ) {
      __SERCOLOR( "Switch off             ", __BRIGHT_YELLOW );
    }
    else if ( AB_state ==  __DISABLED ) {
      __SERPRINT( "Disabled               " );
    }
    __BLANK;
    Serial.println();
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
  __SERPRINT("ROOM:");
    __SERPRINT("\tTemperature: \t\t");
      __SERCOLOR2( aTHA , Tdigit , __BRIGHT_WHITE );
      __BLANK;
      __SERPRINT("\t");
      __SERPRINT(" - Min/Max:\t\t");
      __SERCOLOR2( mTHA , Tdigit , __BRIGHT_WHITE );
      __SERPRINT("/");
      __SERCOLOR2( MTHA , Tdigit , __BRIGHT_WHITE );
      __BLANK;
    __SERPRINTLN();
  __SERPRINTLN();

  // ------------------------------------------------------------------------------------------------------------------------
  Serial.print("CASE:");
    Serial.print("\tFan status:\t\t");
      if ( Rfr == __ON ) {
        __SERBLC( "ON ",  __BRIGHT_GREEN );
      }
      else {
        __SERPRINT( "Off" );
      }
      Serial.print("\t");
      Serial.print(" - Percentage:\t\t");
      __SERCE2( pTHE , 0.00 , pTHE , Tdigit , __BRIGHT_BLUE )
      else __SERBLCH2( pTHE , 90.0 , pTHE , Tdigit , __BRIGHT_RED )
      else __SERBLCH2( pTHE , 80.0 , pTHE , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( pTHE , Tdigit );
      }
      __SERPRINT( "%" );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Activation:\t\t");
      __SERPRINT2( THE_ACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Activation gap:\t");
      __SERCL( abs( THE_ACTIVATION - aTHE ) , 0 , "Hihger", __BRIGHT_RED )
      else __SERBLCL2( abs( THE_ACTIVATION - aTHE ) , 1 ,  ( THE_ACTIVATION - aTHE ) , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( THE_ACTIVATION - aTHE , Tdigit );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTemperature:\t\t");
      __SERCOLOR2( aTHE , Tdigit , __BRIGHT_WHITE );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Derivative:\t\t");
      __SERCH2( ( aTHEDrv * 1000 ), 40.00, ( aTHEDrv * 1000 ), Tdigit, __BRIGHT_RED )
      else __SERCH2( ( aTHEDrv * 1000 ), 30.00, ( aTHEDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else __SERCL2( ( aTHEDrv * 1000 ), -40.00, ( aTHEDrv * 1000 ), Tdigit, __BRIGHT_BLUE )
      else __SERCL2( ( aTHEDrv * 1000 ), -30.00, ( aTHEDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else {
        __SERCOLOR2( ( aTHEDrv * 1000 ) , Tdigit, __BRIGHT_WHITE );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTh Deactivation:\t");
      __SERPRINT2( THE_DEACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Deactivation gap:\t");
      __SERCL( aTHE - THE_DEACTIVATION, 0, "Lower", __BRIGHT_BLUE)
      else __SERPRINT2( aTHE - THE_DEACTIVATION , Tdigit );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Minimum:\t\t");
      __SERPRINT2( mTHE , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Th Maximum:\t\t");
      __SERCH2( MTHE, THE_ACTIVATION, MTHE, Tdigit, __BRIGHT_RED )
      else {
        __SERPRINT2( MTHE , Tdigit );
      }
      __BLANK;
    Serial.println();
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
  Serial.print("VIDEO:");
    Serial.print("\tFan status:\t\t");
      if ( Rvd == __ON ) {
        __SERBLC( "ON ",  __BRIGHT_GREEN );
      }
      else {
        __SERPRINT( "Off" );
      }
      Serial.print("\t");
      Serial.print(" - Percentage:\t\t");
      __SERCE2( pTHV , 0.00 , pTHV , Tdigit , __BRIGHT_BLUE )
      else __SERBLCH2( pTHV , 90.0 , pTHV , Tdigit , __BRIGHT_RED )
      else __SERBLCH2( pTHV , 80.0 , pTHV , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( pTHV , Tdigit );
      }
      __SERPRINT( "%" );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Activation:\t\t");
      __SERPRINT2( THV_ACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Activation gap:\t");
      __SERCL( abs( THV_ACTIVATION - aTHV ) , 0 ,   "Hihger", __BRIGHT_RED )
      else __SERBLCL2( abs( THV_ACTIVATION - aTHV ) , 1 ,  ( THV_ACTIVATION - aTHV ) , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( THV_ACTIVATION - aTHV , Tdigit );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTemperature:\t\t");
      __SERCOLOR2( aTHV , Tdigit , __BRIGHT_WHITE );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Derivative:\t\t");
      __SERCH2( ( aTHVDrv * 1000 ), 40.00, ( aTHVDrv * 1000 ), Tdigit, __BRIGHT_RED )
      else __SERCH2( ( aTHVDrv * 1000 ), 30.00, ( aTHVDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else __SERCL2( ( aTHVDrv * 1000 ), -40.00, ( aTHVDrv * 1000 ), Tdigit, __BRIGHT_BLUE )
      else __SERCL2( ( aTHVDrv * 1000 ), -30.00, ( aTHVDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else {
        __SERCOLOR2( ( aTHVDrv * 1000 ) , Tdigit, __BRIGHT_WHITE );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTh Deactivation:\t");
      __SERPRINT2( THV_DEACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Deactivation gap:\t");
      __SERCL( aTHV - THV_DEACTIVATION, 0, "Lower", __BRIGHT_BLUE)
      else __SERPRINT2( aTHV - THV_DEACTIVATION , Tdigit );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Minimum:\t\t");
      __SERPRINT2( mTHV , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Th Maximum:\t\t");
      __SERCH2( MTHV, THV_ACTIVATION, MTHV, Tdigit, __BRIGHT_RED )
      else {
        __SERPRINT2( MTHV , Tdigit );
      }
      __BLANK;
    Serial.println();
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
  Serial.print("CPU:");
    Serial.print("\tFan status:\t\t");
      if ( Rrr == __ON ) {
        __SERBLC( "ON ",  __BRIGHT_GREEN );
      }
      else {
        __SERPRINT( "Off" );
      }
      Serial.print("\t");
      Serial.print(" - Percentage:\t\t");
      __SERCE2( pTHC , 0.00 , pTHC , Tdigit , __BRIGHT_BLUE )
      else __SERBLCH2( pTHC , 90.0 , pTHC , Tdigit , __BRIGHT_RED )
      else __SERBLCH2( pTHC , 80.0 , pTHC , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( pTHC , Tdigit );
      }
      __SERPRINT( "%" );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Activation:\t\t");
      __SERPRINT2( THC_ACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Activation gap:\t");
      __SERCL( abs( THC_ACTIVATION - aTHC ) , 0 ,   "Hihger", __BRIGHT_RED )
      else __SERBLCL2( abs( THC_ACTIVATION - aTHC ) , 1 ,  ( THC_ACTIVATION - aTHC ) , Tdigit , __BRIGHT_YELLOW )
      else {
        __SERPRINT2( THC_ACTIVATION - aTHC , Tdigit );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTemperature:\t\t");
      __SERCOLOR2( aTHC , Tdigit , __BRIGHT_WHITE );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Derivative:\t\t");
      __SERCH2( ( aTHCDrv * 1000 ), 40.00, ( aTHCDrv * 1000 ), Tdigit, __BRIGHT_RED )
      else __SERCH2( ( aTHCDrv * 1000 ), 30.00, ( aTHCDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else __SERCL2( ( aTHCDrv * 1000 ), -40.00, ( aTHCDrv * 1000 ), Tdigit, __BRIGHT_BLUE )
      else __SERCL2( ( aTHCDrv * 1000 ), -30.00, ( aTHCDrv * 1000 ), Tdigit, __BRIGHT_YELLOW )
      else {
        __SERCOLOR2( ( aTHCDrv * 1000 ) , Tdigit, __BRIGHT_WHITE );
      }
      __BLANK;
    Serial.println();
    Serial.print("\tTh Deactivation:\t");
      __SERPRINT2( THC_DEACTIVATION , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Deactivation gap:\t");
      __SERCL( aTHC - THC_DEACTIVATION, 0, "Lower", __BRIGHT_BLUE)
      else __SERPRINT2( aTHC - THC_DEACTIVATION , Tdigit );
      __BLANK;
    Serial.println();
    Serial.print("\tTh Minimum:\t\t");
      __SERPRINT2( mTHC , Tdigit );
      __BLANK;
      Serial.print("\t");
      Serial.print(" - Th Maximum:\t\t");
      __SERCH2( MTHC, THC_ACTIVATION, MTHC, Tdigit, __BRIGHT_RED )
      else {
        __SERPRINT2( MTHC , Tdigit );
      }
      __BLANK;
    Serial.println();
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
  __SERPRINT( "\tInsert a valid command [h for help] : " );
  if ( inByte ) {
    __SERCOLOR( char(inByte) , __BRIGHT_YELLOW );
    __BLANK;
    delay(1000);
  }
  else
    __SERPRINT( "_" );
  Serial.println();

  // ------------------------------------------------------------------------------------------------------------------------
}


void serOutput( void ) {
  if ( __TERMINAL != __PLOT ) {
    if ( __CONTENT == __LINE ) {
      printLine();
    }
    else if ( __CONTENT == __TABLE ) {
      static unsigned long Tbupd = 0;
      if ( ( millis() - Tbupd ) > 30000 ) {
        clearScreen();
        Tbupd = millis();
      }
      printTable();
    }
  }
  else {
    DISPLAY( RFC_VERSION );

    #if __CONTENT == __TEMP 
      MONITOR2( "Room" , aTHA );
      MONITOR2( "Case" , aTHE );
      MONITOR2( "Video" , aTHV );
      MONITOR2( "Cpu" , aTHC );

      DISPLAY2( "Room" , aTHA );
      DISPLAY2( "Case" , aTHE );
      DISPLAY2( "Video" , aTHV );
      DISPLAY2( "Cpu" , aTHC );
      DISPLAY2( "Derivative" , aTHDrv * 1000 );
    #elif __CONTENT == __DRVS
      if ( ( minutes > 0 ) || ( hours > 0 ) ) {
        MONITOR2( "DrvE" , aTHEDrv * 1000 );
        MONITOR2( "DrvV" , aTHVDrv * 1000 );
        MONITOR2( "DrvC" , aTHCDrv * 1000 );
        MONITOR2( "DrvTOT" , aTHDrv * 1000 );

        DISPLAY2( "Room" , aTHA );
        DISPLAY2( "Case" , aTHE );
        DISPLAY2( "Video" , aTHV );
        DISPLAY2( "Cpu" , aTHC );
      }
    #endif

    MONITOR_ENDL();
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
    Serial.print  ( "Reflection Fan contr [RFC v");
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
    for ( int i = 0; i < 30; i++) 
      Serial.println();
  }
  else
   Serial.println( "  Press h o H to get the commands list" );
}


void  clearScreen( void ) {
  Serial.println( "" );
  if ( cycles > 0 ) {
    if ( (  __SCROLL == __OFF ) && ( __TERMINAL != __INTERNAL ) )
      Serial.println( "\e[1J" );
    for ( int i = 0; i < 100; i++) 
      Serial.println();
  }
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


void tempRead( void ) {
  sensors.requestTemperatures();
  THV_s[ id ] = sensors.getTempC( THvideo );
  THE_s[ id ] = sensors.getTempC( THenvrm );
  THC_s[ id ] = cpu->readCelsius();
  //  ---------------------------------------  MAX values update 
  if ( aTHE > MTHE ) MTHE = aTHE;
  if ( aTHV > MTHV ) MTHV = aTHV;
  if ( aTHC > MTHC ) MTHC = aTHC;
  if ( THE_s[ id ] < mTHE ) mTHE = THE_s[ id ];
  if ( THV_s[ id ] < mTHV ) mTHV = THV_s[ id ];
  if ( THC_s[ id ] < mTHC ) mTHC = THC_s[ id ];

  #if ( __AMB_AVERAGED == __YES )
    THA_s[ id ] = sensors.getTempC( THambnt );
    if ( THA_s[ id ] < mTHA ) mTHA = THA_s[ id ];
    if ( THA_s[ id ] > MTHA ) MTHA = THA_s[ id ];
  #else
    if ( time_amb == __RESET )
      time_amb = millis();
    else if ( ( millis() - time_amb ) >= __AMB_TIME ) {
      aTHA = sensors.getTempC( THambnt );
      time_amb = __RESET;
    }
    if ( aTHA < mTHA ) mTHA = aTHA;
    if ( aTHA > MTHA ) MTHA = aTHA;
  # endif

  for( int a = 0; a < TH_AVERAGE ; a++ ) {
    aTHE += THE_s[a];
    aTHV += THV_s[a];
    aTHC += THC_s[a];
    #if ( __AMB_AVERAGED == __YES )
      aTHA += THA_s[a];
    #endif
  }
  #if ( __AMB_AVERAGED == __YES )
    aTHA /= (float)TH_AVERAGE;
  #endif
  aTHE /= (float)TH_AVERAGE;
  aTHV /= (float)TH_AVERAGE;
  aTHC /= (float)TH_AVERAGE;

  //  --------------------------------------- Drv
  if ( AutoInit == __ON ) {
    THEDrv_s[ iddrv ] = aTHE - oTHE;
    for( int a = 0; a < DRV_AVERAGE ; a++ ) {
      aTHEDrv += THEDrv_s[a];
    }
    aTHEDrv /= (float)DRV_AVERAGE;
    
    THVDrv_s[ iddrv ] = aTHV - oTHV;
    for( int a = 0; a < DRV_AVERAGE ; a++ ) {
      aTHVDrv += THVDrv_s[a];
    }
    aTHVDrv /= (float)DRV_AVERAGE;
    
    THCDrv_s[ iddrv ] = aTHC - oTHC;
    for( int a = 0; a < DRV_AVERAGE ; a++ ) {
      aTHCDrv += THCDrv_s[a];
    }
    aTHCDrv /= (float)DRV_AVERAGE;
    
    THDrv_s[ iddrv ] = ( (float)( aTHEDrv + aTHVDrv + aTHCDrv ) / 3.0 ) - oTHDrv;
    for( int a = 0; a < DRV_AVERAGE ; a++ ) {
      aTHDrv += THDrv_s[a];
    }
    aTHDrv /= (float)DRV_AVERAGE;

    oTHE = aTHE;
    oTHV = aTHV;
    oTHC = aTHC;
    oTHDrv = aTHDrv;

  }
  else {
    iddrv = 0;
  }

  //  ---------------------------------------  Indexes: increments and limits
  if ( ++id >= TH_AVERAGE )
    id = 0;
  if ( ++iddrv >= DRV_AVERAGE )
    iddrv = 0;
}


void airBomb_controller( void ) {
  if ( __AIR_BOMB ==  __ON ) {
    if ( AB_state ==  __INACTIVE ) {
      if ( ( pTHE > __AB_THRESHOLD ) && ( pTHV > __AB_THRESHOLD ) && ( pTHC > __AB_THRESHOLD ) ) {
//      if ( pTHGen > 5.0 ) {
        if ( ( Rvd == __OFF ) && ( Rrr == __OFF ) && ( Rfr == __OFF ) ) {
          AB_time = millis();
          AB_state = __CHARGING;
        }
      }
    }
    else if ( AB_state ==  __CHARGING ) {
      if ( pTHGen < 15.0 )
        AB_state = __INACTIVE;
      else if ( ( Rvd == __ON ) || ( Rrr == __ON ) || ( Rfr == __ON ) ) {
        AB_state = __INACTIVE;
      }
      else {
        if ( ( ( millis() - AB_time ) / 1000 ) > ( __AB_TIMEOUT * 60 ) )
          AB_state = __IGNITION;
      }
    }
    else if ( AB_state == __RUNNING ) {
      if ( pTHGen == 0.0 )
        AB_state = __SWITCH_OFF;
    }
  }
  else
    AB_state ==  __DISABLED;
}


void  Controller( void ) {
  static byte first = __ON;
  if ( cycles < 15 ) {
    AutoInit = __OFF;
  }
  else {
    AutoInit = __ON;
  }

  if ( AutoInit == __ON ) {
    if ( Force ==  __OFF ) {
      contr = __ON;
    }
    else {
      contr = __OFF;
    }
      
    if ( contr == __ON ) {
#if ( __AIR_BOMB ==  __ON )     
      if ( ( first ==  __ON ) && ( pTHGen > 30 ) ) {
        Force = 1;
        SForce = 0;
        AB_state = __RUNNING;
        first = __OFF;
      }
#endif
      Relay_manager( __NOT_USED , __OFF );
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
      else if ( ( aTHC < THC_DEACTIVATION ) && ( aTHV < THV_DEACTIVATION ) ) {
        if ( REnable == __ENABLE ) Relay_manager( __REAR , __OFF );
      }
      
      if ( AB_state ==  __IGNITION ) {
        Force = 1;
        SForce = 0;
        AB_state = __RUNNING;
      }
    }
    else {
      if ( Force ) {
        if ( ( pTHE ==  0.0 ) && ( pTHV == 0.0 ) && ( pTHC == 0.0 ) )
          Force = __OFF;
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
      if ( AB_state ==  __SWITCH_OFF ) {
        Force = 0;
        VEnable = 0;
        REnable = 0;
        FEnable = 0;
        NEnable = 0;
        SForce = 0;
        AB_state = __INACTIVE;
      }
    }
  }
  pTHA = ( ( constrain( aTHA, THA_MIN,  THA_MAX ) - THA_MIN ) / ( THA_MAX - THA_MIN ) ) * 100.0;
  pTHE = ( ( constrain( aTHE, THE_DEACTIVATION,  THE_ACTIVATION ) - THE_DEACTIVATION ) / ( THE_ACTIVATION - THE_DEACTIVATION ) ) * 100.0;
  pTHV = ( ( constrain( aTHV, THV_DEACTIVATION,  THV_ACTIVATION ) - THV_DEACTIVATION ) / ( THV_ACTIVATION - THV_DEACTIVATION ) ) * 100.0;
  pTHC = ( ( constrain( aTHC, THC_DEACTIVATION,  THC_ACTIVATION ) - THC_DEACTIVATION ) / ( THC_ACTIVATION - THC_DEACTIVATION ) ) * 100.0;
  pTHGen = ( pTHE + pTHV + pTHC ) / 3.0;
}


void commandAcq( void ) {
  if (Serial.available()) {
    inByte = Serial.read();
  }
  else
    inByte = 0;
}


void commandProcess( void ) {
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
    case '.': {
      clearScreen();
      break;
    }
    case '+': {
      if ( ++Tdigit > 2 ) Tdigit = 2;
      break;
    }
    case '-': {
      if ( --Tdigit < 1 ) Tdigit = 1;
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


void ledController( void ) {
  
  _L_ON_ON;
  
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

  if ( contr == __ON ) {
    if ( nrun % 2 )
      _L_CNTR_AUTO;
    else
      set_ls_temp( _LN_CNTR , pTHGen );
  }
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
    set_ls_temp( _LN_SA , pTHA );
    set_ls_temp( _LN_SE , pTHE );
    set_ls_temp( _LN_SV , pTHV );
    set_ls_temp( _LN_SC , pTHC );
    
  } else {
    if ( nrun % 2 )
        _L_CNTR_INIT;
      else
        _L_CNTR_OFF;
   }

  if ( ( minutes > 0 ) || ( hours > 0 ) ) {
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
      Serial.print("\033[?25l");
      Serial.println( "\e[1J" );
      Serial.print("\e[2J");
    }
    Serial.println();
    Serial.println("---------------------------------------------------");
    Serial.println();
    Serial.print  ("Proxima Fan contr [PFC v");
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
    _L_SET( 8 , _L_0XFF , _L_0XFF , 0 );
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
    _L_SET( 7 , _L_0XFF , _L_0XFF , 0 );
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
    _L_SET( 6 , _L_0XFF , _L_0XFF , 0 );
    _L_UPDATE;
    delay( 500 );

    pinMode( A1 , INPUT );
    if ( __TERMINAL != __PLOT ) {
      Serial.println();
      Serial.print("Thermal NTC Sensor Initializing [103AQ3]...");
      if ( analogRead( A1 ) > 20 ) {
        Serial.print("Found!");
      }
      else {
        Serial.print("NOT found!");
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
      Serial.println();
      Serial.print("  Current analog read resolution: ");
      Serial.print( __MAX_ANALOG_RES );
      Serial.println();
      Serial.print("  pinMode A1: INPUT: ");
      Serial.print( analogRead( A1 ) ,HEX );
      Serial.println();
    }
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
//    aTHC = cpu->readCelsius();
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
//  if ( __TERMINAL != __PLOT )
//    showHelp();
  _L_SET( 8 , 0 , 0 , 0 );
  _L_SET( 7 , 0 , 0 , 0 );
  _L_SET( 6 , 0 , 0 , 0 );
  _L_SET( 5 , 0 , 0 , 0 );
  _L_UPDATE;
//  delay(2000);
  if ( __TERMINAL != __PLOT ) {
    if ( __CONTENT == __TABLE ) {
      for( byte a = 0; a < 40 ; a++ )
        Serial.println();
      Serial.println( "\e[1J" );
      Serial.print("\e[2J");
    }
    else
      Serial.println( "" );
  }
}



//  --------------------------------------------------------------------- LOOP
void loop(void) {
  unsigned long tTexe = millis();
  
  commandAcq();
  tempRead();
#if ( __AIR_BOMB == __ON )
  airBomb_controller();
#endif
  Controller();
  ledController();
  serOutput();
  if ( __ERROR_CHECK == __ENABLE )
    SensErrorCheck();
  commandProcess();
  
  
  //  ------------------------------------------------------------------- Delay
  cycles = millis() / 1000;
  time( cycles );
  int dly = RFC_DELAY - ( millis() - tTexe );
  if ( dly < 0 ) dly = 1;
  delay( dly );
  Texe = ( millis() - tTexe );
  nrun++;
}
