/*
  Beacon Project
  Written by W7GLF

  Free for public use.
  
 */
 
 
//**************************************************//
//   Define the REPEATER ID below                   //
//**************************************************//
char BeaconID_MorseCode[] = "W7GLF/B";


// Define the input and output pins
#define morse_output_pin 13         // pin for MORSE keying
#define morse_led_pin LED_BUILTIN   // pin for MORSE LED (on board)
#define id_audio_output_pin 10      // output audio on pin 10
#define ptt_output_pin 5            // PTT

// Send Dashes rather than solid carrier between IDs
#define SEND_DASHES true

// Define PTT is active when HIGH or active when LOW
#define PTT_ACTIVE LOW

// Define KEY is active when HIGH or active when LOW
#define CW_ACTIVE LOW

int note = 1200;      // music note/pitch

// Define timers in milliseconds

// ID_TIMER - how long to send carrier between IDs
const unsigned long ID_TIMER = (unsigned long)  30 * 1000;            // 30 seconds

//  Adjust the 'dotlen' length to speed up or slow down the morse code
int dotLen = 100;            // length of the morse code 'dot'

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
    0B101000,           // & use for WAIT    
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
    0,                  // \    
    0,                  // ]    
    0,                  // ^    
    0,                  // _    
};  


// Morse code routines

void do_ident()
{
  for (int i = 0; i < sizeof(BeaconID_MorseCode) - 1; i++)
  {
    // Get the character in the current position
    unsigned char Char = BeaconID_MorseCode[i];
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
  tone(id_audio_output_pin, note, dotLen);    // start playing a tone
  digitalWrite(morse_led_pin, HIGH);          // turn the LED on 
  delay(dotLen);                              // hold in this position
  digitalWrite(morse_output_pin, !CW_ACTIVE); // KEY UP
  digitalWrite(morse_led_pin, LOW);           // turn the LED off 
  noTone(id_audio_output_pin);                // stop playing a tone
  delay(elemPause);              // hold in this position
}

// DASH
void MorseDash()
{
  digitalWrite(morse_output_pin, CW_ACTIVE);    // KEY DOWN 
  tone(id_audio_output_pin, note, dashLen);     // start playing a tone
  digitalWrite(morse_led_pin, HIGH);            // turn the LED on 
  delay(dashLen);                               // hold in this position
  digitalWrite(morse_output_pin, !CW_ACTIVE);   // KEY UP 
  digitalWrite(morse_led_pin, LOW);             // turn the LED off 
  noTone(id_audio_output_pin);                  // stop playing a tone
  delay(elemPause);              // hold in this position
}

void KeyUp()
{
  digitalWrite(morse_output_pin, !CW_ACTIVE);   // KEY UP 
  digitalWrite(morse_led_pin, LOW);             // turn the LED off 
  noTone(id_audio_output_pin);   // stop playing a tone
}

void KeyDown()
{
  digitalWrite(morse_output_pin, CW_ACTIVE);    // KEY DOWN 
  digitalWrite(morse_led_pin, HIGH);            // turn the LED on 
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
  }
  delay (charSpace);
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

//////////////////////////////////////////////////////
// the setup routine runs once when you press reset //
//////////////////////////////////////////////////////
void setup() {                
  // initialize the digital pin as an output for LED lights.
  pinMode(morse_output_pin, OUTPUT);
  pinMode(ptt_output_pin, OUTPUT);
  digitalWrite (morse_output_pin, !CW_ACTIVE);
  digitalWrite(morse_led_pin, LOW);          // turn the LED off
  // Since this is a beacon PTT is always energized
  digitalWrite(ptt_output_pin, PTT_ACTIVE);
  // Give time for TX to come up
  delay(500);
  KeyDown();
}

//////////////////////////////////////////////////////////
// The loop routine repeatedly runs once setup finishes //
//////////////////////////////////////////////////////////

void loop()
{
  // Is it time for an ID?
  if ((millis() - id_timer) > ID_TIMER)
  {
    id_timer = millis();
    KeyUp ();
    delay (wordSpace);
    do_ident();
    delay (wordSpace);
    keydown = false;
  }
  else if (SEND_DASHES)
  {
    MorseDash();
    delay (wordSpace);
  } 
  else if (!keydown)
  {  // Send long carrier - only call KeyDown if it is not down already
    keydown = true;
    KeyDown ();
  }  
}
