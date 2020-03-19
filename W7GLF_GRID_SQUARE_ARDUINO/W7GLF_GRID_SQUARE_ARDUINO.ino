//**************************************************//
//   Define the REPEATER ID below                   //
//**************************************************//

char BeaconID_MorseCode[] = "W7GLF";
char BeaconGrid_MorseCode [8] = "";

#define NANO true  // true for NANO
#define TEENSY false  // true for Teensy LC
#define UBLOX8 true
#define DEBUG false
#define UseSoftwareSerial false
#define LCD_I2C true

// set this to the hardware serial port you wish to use
#if NANO  
  #if UseSoftwareSerial  
    #include <SoftwareSerial.h>
    #define rxPin 3
    #define txPin 2
    SoftwareSerial mySerial (rxPin,txPin);  // RX, TX
    #define HWSERIAL mySerial // For NANO
  #else
    #define HWSERIAL Serial // For NANO
  #endif
#elif TEENSY
#define HWSERIAL Serial1 // Can use UART2 For Teensy LC
#endif

/*-----( Declare objects )-----*/
// set the LCD address to 0x20 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
#if LCD_I2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD I2C address
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

// Stuff for reading GPS
#if UBLOX8
char lat [13] = "0000.00000 N";
char lon [14] = "00000.00000 W";
char mytime [10] = "000000.00";
#else
char lat [12] = "0000.0000 N";
char lon [13] = "00000.0000 W";
char mytime [11] = "000000.000";
#endif
char lcd_line [17];


bool perform_beacon = false;
bool perform_beacon_timed = false;
bool perform_cq = false;
bool do_even_period;

int rx_ndx;
bool gpgsa = false;
bool gpgga = false;
bool valid = true;
bool ThreeDFix = false;

// Define the input and output pins
#define morse_output_pin 10         // pin for MORSE keying
#define morse_led_pin 5             // pin for MORSE LED (20 ma)
#define id_audio_output_pin 0       // of not 0 output audio pin

// Send Dashes rather than solid carrier between IDs
#define SEND_DASHES false

// Define KEY is active when HIGH or active when LOW
#define CW_ACTIVE HIGH

int note = 1200;      // music note/pitch

// Define timers in milliseconds

// ID_TIMER - how long to send carrier between IDs
const unsigned long ID_TIMER = (unsigned long)  30 * 1000;            // 30 seconds

//  Adjust the 'dotlen' length to speed up or slow down the morse code
int dotLen = 90;            // length of the morse code 'dot'

// Timers for ID and HEARTBEAT
unsigned long id_timer;

// Define state variables
boolean keydown = false;

/*
  Define the speed of the morse code
  Adjust the 'dotlen' length to speed up or slow down your morse code
    (all of the other lengths are based on the dotlen)

  Here are the ratios code elements:
    Dash length = Dot length x 3
    Pause between elements = Dot length
      (pause between dots and dashes within the character)
    Pause between characters = Dot length x 3
    Pause between words = Dot length x 7
*/
int dashLen = dotLen * 3;    // length of the morse code 'dash'
int elemPause = dotLen;      // length of the pause between elements of a character
int charSpace = dotLen * 3;  // length of the spaces between characters
int wordSpace = dotLen * 7;  // length of the pause between words

// Each byte has a one bit before character begins as a flag and the rest of the bits are 1 for
// dash and 0 for dot.

 
unsigned char MORSE [64] = {
    0x00, // (space) - special case - handled by do_cw
    0B1101011,          // !    
    0B1010010,          // "    
    0B1000101,          // # use for SK    
    0B10001001,         // $    
    0,                  // %    
    0B101000,           // & use for WAIT AS
    0B1011110,          // '    
    0B110110,           // (    
    0B1101101,          // )    
    0,                  // *    
    0B110101,           // +    
    0B1110011,          // ,    
    0B1100001,          // -    
    0B1010101,          // .    
    0B110010,           // /    
    0B111111,           // 0    
    0B101111,           // 1    
    0B100111,           // 2    
    0B100011,           // 3    
    0B100001,           // 4    
    0B100000,           // 5    
    0B110000,           // 6    
    0B111000,           // 7    
    0B111100,           // 8    
    0B111110,           // 9    
    0B1111000,          // :    
    0B1101010,          // ;    
    0,                  // <    
    0B110001,           // =    
    0,                  // >    
    0B1001100,          // ?    
    0B1011010,          // @    
    0B101,              // A    
    0B11000,            // B    
    0B11010,            // C    
    0B1100,             // D    
    0B10,               // E    
    0B10010,            // F    
    0B1110,             // G    
    0B10000,            // H    
    0B100,              // I    
    0B10111,            // J    
    0B1101,             // K    
    0B10100,            // L    
    0B111,              // M    
    0B110,              // N    
    0B1111,             // O    
    0B10110,            // P    
    0B11101,            // Q    
    0B1010,             // R    
    0B1000,             // S    
    0B11,               // T    
    0B1001,             // U    
    0B10001,            // V    
    0B1011,             // W    
    0B11001,            // X    
    0B11011,            // Y    
    0B11100,            // Z    
    0,                  // [    
    0,                  // BackSlash    
    0,                  // ]    
    0,                  // ^    
    0,                  // _    
};  

void CONVGRID (char lat[], char lon[])
{
//
//  Here is the hard routine Convert lat/lon into 6 digit grid square
//
// The formula ia ABCDEF
// where A,C,E are for longtitude and B,D,F are for latitude
//
// GPS puts out lat/lon in format DDMM.MMMMM (E/W) DDDMM.MMMM (N/S)
//
// To calculate first convert to latitude / longtitude to 0 based:
//
//  if (north)
//    lat = 90 + latitude
//  else
//    lat = 90 - latitude 
//
//  if (east)
//    lon = 180 + longtitude
//  else
//    lon = 180 - longtitude 
//
//  for example 4742.6631 N 12213.4397 W would be
//  lat 137 degrees 42.6631 minutes 
//  lon 57 degrees 47.5603 minutes
//      latMin 42
//      lonMin 86
//
// For first 4 are easy we can just use degrees
//
// A is (lonDeg div 20) + 'A'          (57 div 20 = 2 so 'A' + 2 = 'C')
// B is (latDeg div 10) + 'A'          (137 div 10 = 13 so 'A' + 13 = 'N') 
// C is ((lonDeg mod 20) div 2) + '0'  (57 mod 20 = 17, 17 div 2 = 8)
// D is (latDeg mod 10)   + '0'        (137 mod 10 = 7)
//
// For last two we need to use minutes also (lon is hard one)
//
// E is (((lonDeg mod 2) * 60) + lonMin) div 5 ((1*60 + 47 = 107)/5 = 21, so 'A' + 21 = 'V')
// F is (int) (latMin / 2.5)           (42 / 2.5 = 16, so 'A' + 17 = 'R')
//
// Code below is tricky because 8 bit arithmetic limits us to stay below 256
//
int  lat0;          // First digit of latitude - format DDMMFFFF
int  lat1;          // Second digit of latitude
int  lat2;          // Third digit of latitude
int  lat3;          // Fourth digit of latitude

int  lon0;          // First digit of longitude - format DDDMMFFFF 
int  lon1;          // Second digit of longitude
int  lon2;          // Third digit of longitude
int  lon3;          // Fouth digit of longitude

int  rb3;           // Temp to remember borrow 
int  rt;            // temp
int  rt1;           // temp
int  east;          // Is longitude East?
int  north;         // Is latitude North?
 
//
//   =============================================================
//   Do latitude
//   =============================================================

        lat0 = (lat[0] - '0');
        lat1 = (lat[1] - '0'); //lat mod 10

        // Now latitude get minutes 
        lat2 = (lat[2] - '0');

        // To get minutes multiply first digit of minutes by 10
        rt1 = lat2;       // Remember number for times 5
        lat2 <<= 2;       // times 4
        lat2 += rt1;      // now times 5
        lat2 <<= 1;       // now times 10

        // Now add units
        lat2 += (lat[3] - '0');

        lat3 = rb3 =  (lat[5] - '0');  // See if there is a fractional part
        rb3 += (lat[6] - '0'); //  which will cause a borrow from
        rb3 += (lat[7] - '0'); //  degrees into minutes if South
        rb3 += (lat[8] - '0');

#if UBLOX8
        rb3 += (lat[9] - '0');
        north = (lat[11] == 'N');
#else
        north = (lat[10] == 'N');
#endif

        // Do corrections for North or South
        if (north) 
            lat0 += 9;          // (90 + lat) div 10
        else
        {
            // This is the hard one - might be borrow
            lat0 = 9 - lat0;    // (90 - lat) div 10
            if (lat1 != 0 || rb3 != 0)
            {
                // Borrow from tens of degrees
                lat0--;
                lat1 = 10 - lat1;
                if (lat2 != 0 || rb3 != 0)
                {
                    //Borrow from units of degrees
                    lat1--;
                    lat2 = 60 - lat2;
                    if (rb3 != 0)
                    {
                        //Borrow from tens of minutes
                        lat2--;
                        lat3 = 10 - lat3;  // Figure out unit digit of minutes 
                    }
                }
            }
        }
// NorthSouthDone

    // Work on subgrid
        lat2 <<= 1;      // To divide by 2.5 we multiply by 2 and divide by 5

        // Now we need to see if we need to add 1
        // based on whether top bit of fraction of minutes is set
        if (lat3 >= 5) lat2++;  // Add in carry from lower digit

        rt = 0;
        while (lat2 >= 5)
        {  // divide by 5
            lat2 -= 5;
            rt++;
        };
        lat2 = rt;                 // Subgrid letter offset
        if (lat2 == 24) lat2 = 0;  // Should never happen (60 degrees)

//  =============================================================
//  Do longitutde
//  =============================================================

        lon0 = (lon[0] - '0');  // hundreds digit
        lon1 = (lon[1] - '0');  // lon div 10
        if (lon0) lon1 += 10;   // lon1 now equals long div 10

        lon2 = (lon[2] - '0');  // units digit

        rb3 =  (lon[3] - '0');  // See if there is a fractional part
        rb3 += (lon[4] - '0'); //  which will cause a borrow from
        rb3 += (lon[6] - '0'); //  degrees into minutes if we are West
        rb3 += (lon[7] - '0');
        rb3 += (lon[8] - '0');
        rb3 += (lon[9] - '0');
 
#if UBLOX8
        rb3 += (lon[10] - '0');
        east = (lon[12] == 'E');
#else
        east = (lon[11] == 'E');
#endif

        // Do corrections for East or West
        if (east)
        {
            lon1 = 18 + lon1;        // Adding is much easier
        }
        else
        {
            // West is the hard one there might be borrow (rb3 non-zero)
            if (lon2 != 0 || rb3 != 0)   // if units > 0  ** ERC patch ** 
            {                      //    Subtracting from 180 will borrow 
                lon1++;             //   from 10's digit - we can get same
                                   //    result by bumping subtrahend - trick
                                   //    to save some code.
                lon2 = 10 - lon2;    // Subtract units
                if (rb3 != 0) lon2--;    // We need a carry
            }
            lon1 = 18 - lon1;        // Now subtract from 180
        }

// EastWestDone

//   At this point lon1 contains lon div 10 (tens of degrees).
//   First letter is lon1 div 2.
//   First digit is (lon1 mod 2)*10 + lon2

        rt = lon1;       // save value for later
        lon1 >>= 1;      // lon1 is now div 2

        rt &= 1;        // rt is old lon1 mod 2
        if (rt != 0) lon2 += 10; // lon2 is mod 20 (Need to include extra 10 if if it there)

        rt = lon2 & 1;   // For last letter we need to remember (lonDeg mod 2)
        lon2 >>= 1;      // lon2 is now mod 20 div 2


        // The hard part is the last letter
        // Last letter is (((lonDeg mod 2) * 60) + lonMin) div 5
        // For the longitude we need to remember the value modulo 2
        lon3 = (lon[3] - '0');  // Get first decimal place

        // Multiply by 10
        rt1 = lon3;       // Remember number for times 5
        lon3 <<= 2;       // times 4
        lon3 += rt1;      // now times 5
        lon3 <<= 1;       // now times 10

        lon3 += (lon[4] - '0');  // Get second decimal place

        if (!east && rb3 != 0)
        {
            lon3 = 60 - lon3;         // If West subtract minutes from 60
            rb3 = (lon[6] - '0');
            rb3 += (lon[7] - '0');
            rb3 += (lon[8] - '0');
            rb3 += (lon[9] - '0');
            if (rb3 != 0) lon3--;    // We need a carry
        }

        if (rt) lon3 += 60;  // Here is where we add in (lonDeg mod 2) * 60

        rt = 0;
        while (lon3 >= 5)        // Now do div 5 
        {
            lon3 -= 5;          
            rt++;
        };
        lon3 = rt;               // First letter offset
        if (lon3 == 24) lon3 = 0; // Just in case lon minutes is 60 degrees


        // Ready for grid square

        BeaconGrid_MorseCode [0] = lon1 + 'A';
        BeaconGrid_MorseCode [1] = lat0 + 'A';
        BeaconGrid_MorseCode [2] = lon2 + '0';
        BeaconGrid_MorseCode [3] = lat1 + '0';
        BeaconGrid_MorseCode [4] = lon3 + 'a';
        BeaconGrid_MorseCode [5] = lat2 + 'a';
        BeaconGrid_MorseCode [6] = '\0';
#if DEBUG
        Serial.print("\n");
        Serial.write (lon1 + 'A');
        Serial.write (lat0 + 'A');
        Serial.write (lon2 + '0');
        Serial.write (lat1 + '0');
        Serial.write (lon3 + 'a');
        Serial.write (lat2 + 'a');
        Serial.print("\n");
#endif
}

void CONVGRID (long lat, long lon)
{
//
//  Here is the hard routine Convert lat/lon into 6 digit grid square
//
// The formula ia ABCDEF
// where A,C,E are for longtitude and B,D,F are for latitude
//
// GPS puts out lat/lon in format DDMM.MMMMM (E/W) DDDMM.MMMM (N/S)
//
// To calculate first convert to latitude / longtitude to 0 based:
//
//  if (north)
//    lat = 90 + latitude
//  else
//    lat = 90 - latitude 
//
//  if (east)
//    lon = 180 + longtitude
//  else
//    lon = 180 - longtitude 
//
//  for example 4742.6631 N 12213.4397 W would be
//  lat 137 degrees 42.6631 minutes 
//  lon 57 degrees 47.5603 minutes
//      latMin 42
//      lonMin 86
//
// For first 4 are easy we can just use degrees
//
// A is (lonDeg div 20) + 'A'          (57 div 20 = 2 so 'A' + 2 = 'C')
// B is (latDeg div 10) + 'A'          (137 div 10 = 13 so 'A' + 13 = 'N') 
// C is ((lonDeg mod 20) div 2) + '0'  (57 mod 20 = 17, 17 div 2 = 8)
// D is (latDeg mod 10)   + '0'        (137 mod 10 = 7)
//
// For last two we need to use minutes also (lon is hard one)
//
// E is (((lonDeg mod 2) * 60) + lonMin) div 5 ((1*60 + 47 = 107)/5 = 21, so 'A' + 21 = 'V')
// F is (int) (latMin / 2.5) rounded up      (42.663 / 2.5 = 17, so 'A' + 17 = 'R')
//
// Code below is tricky because 8 bit arithmetic limits us to stay below 256
//
long lat_temp;
long lon_temp;

long latDeg;
long lonDeg;
long latMin;
long lonMin;

 long latMinFrac;

int  lat0;          // First digit of latitude - format DDMMFFFF
int  lat1;          // Second digit of latitude
int  lat2;          // Third digit of latitude

int  lon0;          // First digit of longitude - format DDDMMFFFF 
int  lon1;          // Second digit of longitude
int  lon2;          // Third digit of longitude
//
//   =============================================================
//   Do latitude
//   =============================================================

    // Convert from Degrees Minutes to Degrees
           
        latMin = lat % 100000L;
        latDeg = lat - latMin;
        lat_temp = latMin * 5 / 3;
        lat_temp += latDeg;  // lat_temp is now Degrees

    // Now we can convert N/S
        lat_temp += 90*100000L;        

#if DEBUG
        Serial.print ("lat = ");
        Serial.println (lat);
        Serial.print ("lat_temp = ");
        Serial.println (lat_temp);
        Serial.print ("latDeg = ");
        Serial.println (latDeg);
        Serial.print ("latMin = ");
        Serial.println (latMin);
#endif

    // Convert from Degrees to Degrees Minutes
        latMin = lat_temp % 100000L;
        latDeg = lat_temp - latMin;
        lat_temp = latMin * 3 / 5;
        lat_temp += latDeg;  // lat_temp is now Degrees Minutes

        latDeg = lat_temp / 100000L;
        latMin = (lat_temp % 100000L) / 1000L;
        
        latMinFrac = lat_temp % 1000L;

        lat0 = latDeg / 10;
        lat1 = latDeg % 10; 

    // Work on subgrid
        lat2 = (int) (latMin / 2.5);
        if (latMinFrac >= 500) lat2++;

#if DEBUG
        Serial.print ("lat = ");
        Serial.println (lat);
        Serial.print ("lat_temp = ");
        Serial.println (lat_temp);
        Serial.print ("latDeg = ");
        Serial.println (latDeg);
        Serial.print ("latMin = ");
        Serial.println (latMin);
        Serial.print ("latMinFrac = ");
        Serial.println (latMinFrac);
#endif

//  =============================================================
//  Do longitutde
//  =============================================================

    // Convert from Degrees Minutes to Degrees
           
        lonMin = lon % 100000L;
        lonDeg = lon - lonMin;
        lon_temp = lonMin * 5 / 3;
        lon_temp += lonDeg;  // lat_temp is now Degrees

    // Now we can convert E/W
        lon_temp += 180*100000L;        

#if DEBUG
        Serial.print ("lon = ");
        Serial.println (lon);
        Serial.print ("lon_temp = ");
        Serial.println (lon_temp);
        Serial.print ("lonDeg = ");
        Serial.println (lonDeg);
        Serial.print ("lonMin = ");
        Serial.println (lonMin);
#endif

    // Convert from Degrees to Degrees Minutes
        lonMin = lon_temp % 100000L;
        lonDeg = lon_temp - lonMin;
        lon_temp = lonMin * 3 / 5;
        lon_temp += lonDeg;  // lon_temp is now Degrees Minutes

        lonDeg = lon_temp / 100000L;
        lonMin = (lon_temp % 100000L) / 1000L;
        
        lon0 = lonDeg / 20;
        lon1 = (lonDeg % 20) / 2;
//
// For last two we need to use minutes also (lon is hard one)
//
        lon2 = (((lonDeg % 2) * 60) + lonMin) / 5;

#if DEBUG
        Serial.print ("lon = ");
        Serial.println (lon);
        Serial.print ("lon_temp = ");
        Serial.println (lon_temp);
        Serial.print ("lonDeg = ");
        Serial.println (lonDeg);
        Serial.print ("lonMin = ");
        Serial.println (lonMin);
#endif

        // Ready for grid square

        BeaconGrid_MorseCode [0] = lon0 + 'A';
        BeaconGrid_MorseCode [1] = lat0 + 'A';
        BeaconGrid_MorseCode [2] = lon1 + '0';
        BeaconGrid_MorseCode [3] = lat1 + '0';
        BeaconGrid_MorseCode [4] = lon2 + 'a';
        BeaconGrid_MorseCode [5] = lat2 + 'a';
        BeaconGrid_MorseCode [6] = '\0';
#if DEBUG
        Serial.print("\n");
        Serial.write (lon0 + 'A');
        Serial.write (lat0 + 'A');
        Serial.write (lon1 + '0');
        Serial.write (lat1 + '0');
        Serial.write (lon2 + 'a');
        Serial.write (lat2 + 'a');
        Serial.print("\n");
#endif
}

// Morse code routines

void do_ident()
{
  unsigned char Char;
  for (unsigned int i = 0; i < strlen(BeaconID_MorseCode); i++)
  {
    if (!perform_beacon && !perform_beacon_timed) return;
    // Get the character in the current position
    Char = BeaconID_MorseCode[i];
    // Call the subroutine to get the morse code equivalent for this character
    do_cw(Char);
  }
  if (ThreeDFix)
  {
    for (unsigned int i = 0; i < strlen(BeaconGrid_MorseCode); i++)
    {
      if (!perform_beacon && !perform_beacon_timed) return;
      // Get the character in the current position
      Char = BeaconGrid_MorseCode[i];
      // Call the subroutine to get the morse code equivalent for this character
      do_cw(Char);
    }
  }
}

void do_cq()
{
  unsigned char Char;
  char CQ_MorseCode [] = " CQ CQ CQ DE ";
  char CQ_End_MorseCode [] = "K";

  for (int repeat = 0; repeat < 2; repeat++)
  { 
    for (unsigned int i = 0; i < strlen(CQ_MorseCode); i++)
    {
      if (!perform_cq) return;
      // Get the character in the current position
      Char = CQ_MorseCode[i];
      // Call the subroutine to get the morse code equivalent for this character
      do_cw(Char);
    }
  
    for (int repeat1 = 0; repeat1 < 2; repeat1++)
    { 
      for (unsigned int i = 0; i < strlen(BeaconID_MorseCode); i++)
      {
        if (!perform_cq) return;
        // Get the character in the current position
        Char = BeaconID_MorseCode[i];
        // Call the subroutine to get the morse code equivalent for this character
        do_cw(Char);
      }
      delay (wordSpace);
    }
  }
  for (unsigned int i = 0; i < strlen(CQ_End_MorseCode); i++)
  {
    if (!perform_cq) return;
    // Get the character in the current position
    Char = CQ_End_MorseCode[i];
    // Call the subroutine to get the morse code equivalent for this character
    do_cw(Char);
  }
}

// Beep
void DoBeep()
{
  MorseDot ();        
}

// DOT
void MorseDot()
{
  digitalWrite(morse_output_pin, CW_ACTIVE);  // KEY DOWN
  if ( id_audio_output_pin )
     tone(id_audio_output_pin, note, dotLen); // start playing a tone
  digitalWrite(morse_led_pin, HIGH);          // turn the LED on 
  delay(dotLen);                              // hold in this position
  digitalWrite(morse_output_pin, !CW_ACTIVE); // KEY UP
  digitalWrite(morse_led_pin, LOW);           // turn the LED off 
  if ( id_audio_output_pin )
     noTone(id_audio_output_pin);             // stop playing a tone
  delay(elemPause);              // hold in this position
}

// DASH
void MorseDash()
{
  digitalWrite(morse_output_pin, CW_ACTIVE);    // KEY DOWN 
  if ( id_audio_output_pin )
     tone(id_audio_output_pin, note, dashLen);  // start playing a tone
  digitalWrite(morse_led_pin, HIGH);            // turn the LED on 
  delay(dashLen);                               // hold in this position
  digitalWrite(morse_output_pin, !CW_ACTIVE);   // KEY UP 
  digitalWrite(morse_led_pin, LOW);             // turn the LED off 
  if ( id_audio_output_pin )
     noTone(id_audio_output_pin);               // stop playing a tone
  delay(elemPause);              // hold in this position
}

void KeyUp()
{
  digitalWrite(morse_output_pin, !CW_ACTIVE);   // KEY UP 
  digitalWrite(morse_led_pin, LOW);             // turn the LED off 
  if ( id_audio_output_pin )
     noTone(id_audio_output_pin);   // stop playing a tone
}

void KeyDown()
{
  digitalWrite(morse_output_pin, CW_ACTIVE);    // KEY DOWN 
  digitalWrite(morse_led_pin, HIGH);            // turn the LED on 
  if ( id_audio_output_pin )
     tone(id_audio_output_pin, note);  // start playing a tone
}

// *** Characters to Morse Code Conversion *** //
void do_cw(unsigned char tmpChar)
{
  // Take the passed character and use a switch case to find the morse code for that character
  unsigned char element;
  int length;
  int index;

  // Make sure we handle lowercase letters as uppercase in index into our MORSE array.
  index = toupper(tmpChar);

  // Convert ASCII codes 0x20 through 0x5F to indices 0x0 through 0x3F
  index -= 0x20;

  // If value is out of range simply return
  if (index < 0x0 || index > 0x3F) {
      return;
  }
  
  // The value 0 corresponds to a space
  if (index == 0) {
      delay(wordSpace);
      return;
  }

  element = MORSE [index];
  if (element == 0) return;
  length = 8;
  while (--length > 0)
  {
    // Find start of character indicated by a one flag bit
    if (element & 0x80)
    {
      element <<= 1; // Skip flag character      
      break;
    }
    element <<= 1;      
  }
  // Now do the actual character
  while (length > 0)
  {
    if (element & 0x80)
    {
       MorseDash();
    }
    else
    {
       MorseDot();
    }
    length--;
    element <<= 1;
    // Try to update time ...
    update_lcd_time ();            
  }
  delay (charSpace);
}

void serialEvent1() {
  char inchar;
  while(HWSERIAL.available())
  {
    inchar = HWSERIAL.read();
#if DEBUG
    Serial.write (inchar);
#endif
    if (!valid)
    // Search for '$'
    {
      if (inchar == '$')
      {
        valid = true;
        gpgsa = false;
        gpgga = false;
        rx_ndx = 0;
      }
      continue;
    }
    // valid - in string
    rx_ndx++;
    if (rx_ndx == 1)
    {
      if (inchar != 'G')
      {
        valid = false;
      }
      continue;
    }
    if (rx_ndx == 3)
    {
      if (inchar != 'G')
      {
        valid = false;
      }
      continue;
    }
    if (rx_ndx == 4)
    {
      if (inchar == 'S')
      {
        gpgsa = true;
      }
      else if (inchar == 'G')
      {
        gpgga = true;
      }
      else
      {
        valid = false;
      }
      continue;
    }
    if (rx_ndx == 5)
    {
      if (inchar != 'A')
      {
        valid = false;
      }
      continue;
    }
    // At this point string is known to be GxGGA or GxGSA
    // See if we are doing lat
    if (gpgga)
    {
#if UBLOX8
      if (rx_ndx < 7) continue;
      if ((rx_ndx >= 7) && (rx_ndx <= 15))
      {
        mytime [rx_ndx-7] = inchar;
        continue;
      }
      if (rx_ndx == 16) continue;
      if ((rx_ndx >= 17) && (rx_ndx <= 28))
      {
        lat [rx_ndx-17] = inchar;
        continue;
      }
      if (rx_ndx == 29) continue;
      if ((rx_ndx >= 30) && (rx_ndx <= 42))
      {
        lon [rx_ndx-30] = inchar;
        continue;
      }
      if (rx_ndx == 43) 
      {
        CONVGRID (lat, lon);
#if DEBUG
        Serial.println (mytime);
#endif
        continue;
      }
#else
      if (rx_ndx < 7) continue;
      if ((rx_ndx >= 7) && (rx_ndx <= 16))
      {
        mytime [rx_ndx-7] = inchar;
        continue;
      }
      if (rx_ndx == 17) continue;
      if ((rx_ndx >= 18) && (rx_ndx <= 28))
      {
        lat [rx_ndx-18] = inchar;
        continue;
      }
      if (rx_ndx == 29) continue;
      if ((rx_ndx >= 30) && (rx_ndx <= 41))
      {
        lon [rx_ndx-30] = inchar;
        continue;
      }
      if (rx_ndx == 42) 
      {
        CONVGRID (lat, lon);
#if DEBUG
        Serial.println (mytime);
#endif
        continue;
      }
#endif
      // We are done
      valid = false;
    }
    if (gpgsa)
    {
      if (rx_ndx < 9) continue;
      if (rx_ndx == 9)
      {
        ThreeDFix = (inchar == '3');
#if DEBUG
        Serial.print ("ThreeDFix = ");
        Serial.println (ThreeDFix);
#endif
        continue;
      }
      // We are done
      valid = false;
    }
  }
}

void update_lcd () {
     // Update LCD
    lcd_line[0] = lat[0];
    lcd_line[1] = lat[1];
    lcd_line[2] = ' ';
    lcd_line[3] = lat[2];
    lcd_line[4] = lat[3];
    lcd_line[5] = lat[4];
    lcd_line[6] = lat[5];
    lcd_line[7] = ' ';
    lcd_line[8] = mytime[0];
    lcd_line[9] = mytime[1];
    lcd_line[10] = ':';
    lcd_line[11] = mytime[2];
    lcd_line[12] = mytime[3];
    lcd_line[13] = ':';
    lcd_line[14] = mytime[4];
    lcd_line[15] = mytime[5];
    lcd_line[16] = '\0';
    lcd.setCursor(0,0);
    lcd.print(lcd_line);
    
    lcd_line[0] = lon[0];
    lcd_line[1] = lon[1];
    lcd_line[2] = lon[2];
    lcd_line[3] = ' ';
    lcd_line[4] = lon[3];
    lcd_line[5] = lon[4];
    lcd_line[6] = lon[5];
    lcd_line[7] = lon[6];
    lcd_line[8] = ' ';
    lcd_line[9] = ' ';
    lcd_line[10] = BeaconGrid_MorseCode[0];
    lcd_line[11] = BeaconGrid_MorseCode[1];
    lcd_line[12] = BeaconGrid_MorseCode[2];
    lcd_line[13] = BeaconGrid_MorseCode[3];
    lcd_line[14] = BeaconGrid_MorseCode[4];
    lcd_line[15] = BeaconGrid_MorseCode[5];
    lcd_line[16] = '\0';
    lcd.setCursor(0,1);
    lcd.print(lcd_line);
}

void update_lcd_time ()
{
    lcd_line[0] = mytime[0];
    lcd_line[1] = mytime[1];
    lcd_line[2] = ':';
    lcd_line[3] = mytime[2];
    lcd_line[4] = mytime[3];
    lcd_line[5] = ':';
    lcd_line[6] = mytime[4];
    lcd_line[7] = mytime[5];
    lcd_line[8] = '\0';
    lcd.setCursor(8,0);
    lcd.print(lcd_line);  
}

void isr_stop () {
  perform_beacon = false;
  perform_beacon_timed = false;
  perform_cq = false;
}

void isr_beacon () {
  perform_beacon = true;
  perform_beacon_timed = false;
  perform_cq = false;
  id_timer = 0;
}

void isr_beacon_timed () {
  perform_beacon = false;
  perform_beacon_timed = true;
  perform_cq = false;
  do_even_period = ( ((mytime[3]-'0') % 10) < 5 );
  id_timer = 0;
}

void isr_cq () {
  perform_beacon = false;
  perform_beacon_timed = false;
  perform_cq = true;
  id_timer = 0;
}


void setup() {
  // put your setup code here, to run once:

    // initialize the digital pin as an output for CW keying.
  pinMode(morse_output_pin, OUTPUT);
  pinMode(morse_led_pin, OUTPUT);

#if NANO && UseSoftwareSerial
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
#endif  

  pinMode(6, INPUT_PULLUP); // Pushbutton
  attachInterrupt(digitalPinToInterrupt(6), isr_stop, CHANGE);  
  pinMode(7, INPUT_PULLUP); // Pushbutton
  attachInterrupt(digitalPinToInterrupt(7), isr_beacon, CHANGE);  
  pinMode(8, INPUT_PULLUP); // Pushbutton
  attachInterrupt(digitalPinToInterrupt(8), isr_beacon_timed, CHANGE);  
  pinMode(9, INPUT_PULLUP); // Pushbutton
  attachInterrupt(digitalPinToInterrupt(9), isr_cq, CHANGE);  

#if TEENSY
  // Teensy
  Serial.begin (9600); //  Serial to the PC via Serial Monitor at 9600 baud
  Serial.println ("Hello There");
#endif 

  HWSERIAL.begin(9600);

  digitalWrite (morse_output_pin, !CW_ACTIVE);
  digitalWrite(morse_led_pin, LOW);          // turn the LED off

  // Give time for TX to come up
  delay(500);
  KeyUp();

#if 1
//  lcd.begin(16,2);         // initialize the lcd for 16 chars 2 lines and turn on backlight

  #if LCD_I2C
  lcd.init();
  lcd.backlight(); // finish with backlight on  
  #else
  lcd.begin(16,2);
  analogWrite(10,255);
  #endif
  
//-------- Write characters on the display ----------------
// NOTE: Cursor Position: CHAR, LINE) start at 0  
  lcd.setCursor(3,0); //Start at character 4 on line 0
  lcd.print("Hello, world!");
  lcd.setCursor(3,1); //Start at character 4 on line 1
  lcd.print("W7GLF");
#endif

  id_timer = 0;
  delay(100);  
}

void loop() {
  // put your main code here, to run repeatedly
  
#if NANO
  serialEvent1 (); // Teensy LC will call serialEvent1 since it has a UART named Serial1.
#endif

  if (!perform_beacon && !perform_beacon_timed && !perform_cq)
  {
    if (keydown)
    {
      KeyUp ();
      keydown = false;
    }

    if (ThreeDFix)
    {
      update_lcd ();
    }
  }

  if (perform_beacon)
  {      
    // Is it time for an ID?
    if ((millis() + ID_TIMER - id_timer) > ID_TIMER)
    {
      update_lcd ();

      KeyUp ();
      delay (wordSpace);
      do_ident();
      delay (wordSpace);
      id_timer = millis() + ID_TIMER;
      keydown = false;
    }
    else if (SEND_DASHES)
    {
      update_lcd_time ();    

      MorseDash();
      delay (wordSpace);
    } 
    else
    {  // Send long carrier - only call KeyDown if it is not down already
        update_lcd_time ();    
        if (!keydown)
        {  // Send long carrier - only call KeyDown if it is not down already
          keydown = true;
          KeyDown ();
        }
    }
  }

  if (perform_beacon_timed)
  {      
    if ( do_even_period == (((mytime[3]-'0') % 10) < 5) )
    {
      // Is it time for an ID?
      if ((millis() + ID_TIMER - id_timer) > ID_TIMER)
      {
        update_lcd ();

        KeyUp ();
        delay (wordSpace);
        do_ident();
        delay (wordSpace);
        id_timer = millis() + ID_TIMER;
        keydown = false;
      }
      else if (SEND_DASHES)
      {
        update_lcd_time ();    

        MorseDash();
        delay (wordSpace);
      } 
      else 
      {
        update_lcd_time ();    
        if (!keydown)
        {  // Send long carrier - only call KeyDown if it is not down already
          keydown = true;
          KeyDown ();
        }
      }
    }
    else
    {
      update_lcd_time ();    
      if (keydown)
      {  // Send long carrier - only call KeyDown if it is not down already
        keydown = false;
        KeyUp ();
      }      
    }
  }

  if (perform_cq)
  {
    // Is it time for a CQ?
    if ((millis() + ID_TIMER - id_timer) > ID_TIMER)
    {
      update_lcd ();

      KeyUp ();
      do_cq();
      id_timer = millis() + ID_TIMER;
      keydown = false;
    }
    else
    {
      update_lcd_time ();            
    }
  }
}
