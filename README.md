# hamarduino
This respository holds Arduino sketches for ham radio projects

The files are:

W7GLF_BEACON
  Simple program for a beacon

W7GLF_GRID_SQUARE
  Program for an Arduino inside of a BG7TBL 10 MHz reference to
  decode GPS output into a 6 digit grid square.  Also included is a CW beacon keyer which can send CQ and BEACON type messages.
  
  Included are some photos that demonstrate how the installation was done.  The 4 buttons on the back are STOP, SHORT BEACON, TIMED BEACON, ABD CQ.
  The timed beacon sends a beacon for 5 minutes and then receives for 5 minutes.  It synchronizes with the time stamp modulo 300 seconds starting when the button is pressed.  The idea is to run with another station which is also coordinated on 5 minute intervals for rain bounce.
  
  The processor I used is a Teensy LC but a NANO could be used as well however the Arduino board 5V pin should be used rather than Vin.  Note also there is a define in the code because the output from the NEO8 GPS has a different number of digits than the NEO7 GPS.
  
  I have not tested with an Nano but it should work.
  