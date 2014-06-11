// Super-basic Trellis MIDI example.  Maps Trellis buttons to MIDI
// note on/off events; this is NOT a sequencer or anything fancy.
// Requires a PJRC Teensy or an Arduino Leonardo w/TeeOnArdu config,
// software on host computer for synth or to route to other devices.

#include <Wire.h>
#include <Adafruit_Trellis.h>

#define LED     13 // Pin for heartbeat LED (shows code is working)
#define CHANNEL 1  // MIDI channel number
// For this example, MIDI note numbers are simply centered based on
// the number of Trellis buttons (64 in this case); each row doesn't
// necessarily correspond to an octave or anything.
#define LOWNOTE ((127 - sizeof(i2xy)) / 2)

// The TrellisSet for this example is comprised of four Trellises in
// a 2x2 arrangement (8x8 buttons total).  addr[] is the I2C address
// of the upper left, upper right, lower left and lower right matrices,
// respectively, assuming an upright orientation, i.e. labels on board
// are in the normal reading direction.
Adafruit_Trellis    T[4];
Adafruit_TrellisSet trellis(&T[0], &T[1], &T[2], &T[3]);
const uint8_t       addr[] = { 0x70, 0x71, 0x72, 0x73 };

uint8_t       heart        = 0;  // Heartbeat LED counter
unsigned long prevReadTime = 0L; // Keypad polling timer

// The following tables aren't extensively used by this demo, but can
// simplify writing sequencers and instruments.  i2xy converts a button
// index (0-63) to an X/Y pair (high 4 bits are column, low 4 are row).
// xy2i is the opposite, remapping a column (0-7) & row (0-7) to a button
// or LED index.  Both reside in flash space; to access, use
// xy=pgm_read_byte(&i2xy[i]) or i=pgm_read_byte(&xy2i[x][y])
static const uint8_t PROGMEM
  i2xy[] = { // Remap 8x8 TrellisSet button index to column/row
    0x00, 0x10, 0x20, 0x30, 0x01, 0x11, 0x21, 0x31,
    0x02, 0x12, 0x22, 0x32, 0x03, 0x13, 0x23, 0x33,
    0x40, 0x50, 0x60, 0x70, 0x41, 0x51, 0x61, 0x71,
    0x42, 0x52, 0x62, 0x72, 0x43, 0x53, 0x63, 0x73,
    0x04, 0x14, 0x24, 0x34, 0x05, 0x15, 0x25, 0x35,
    0x06, 0x16, 0x26, 0x36, 0x07, 0x17, 0x27, 0x37,
    0x44, 0x54, 0x64, 0x74, 0x45, 0x55, 0x65, 0x75,
    0x46, 0x56, 0x66, 0x76, 0x47, 0x57, 0x67, 0x77 },
  xy2i[8][8] = { // Remap col/row to Trellis button/LED index
    {  0,  4,  8, 12, 32, 36, 40, 44 },
    {  1,  5,  9, 13, 33, 37, 41, 45 },
    {  2,  6, 10, 14, 34, 38, 42, 46 },
    {  3,  7, 11, 15, 35, 39, 43, 47 },
    { 16, 20, 24, 28, 48, 52, 56, 60 },
    { 17, 21, 25, 29, 49, 53, 57, 61 },
    { 18, 22, 26, 30, 50, 54, 58, 62 },
    { 19, 23, 27, 31, 51, 55, 59, 63 } };

void setup() {
  pinMode(LED, OUTPUT);
  trellis.begin(addr[0], addr[1], addr[2], addr[3]);
#ifdef __AVR__
  // Default Arduino I2C speed is 100 KHz, but the HT16K33 supports
  // 400 KHz.  We can force this for faster read & refresh, but may
  // break compatibility with other I2C devices...so be prepared to
  // comment this out, or save & restore value as needed.
  TWBR = 12;
#endif
  trellis.clear();
  trellis.writeDisplay();
}

void loop() {
  unsigned long t = millis();
  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(trellis.readSwitches()) {  // Button state change?
      for(uint8_t i=0; i<sizeof(i2xy); i++) { // For each button...
        // Get column/row for button, convert to MIDI note number
        uint8_t xy   = pgm_read_byte(&i2xy[i]),
                note = LOWNOTE + (xy & 0x0F) * 8 + (xy >> 4);
        if(trellis.justPressed(i)) {
          usbMIDI.sendNoteOn(note, 127, CHANNEL);
          trellis.setLED(i);
        } else if(trellis.justReleased(i)) {
          usbMIDI.sendNoteOff(note, 0, CHANNEL);
          trellis.clrLED(i);
        }
      }
      trellis.writeDisplay();
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); // Blink = alive
  }
  while(usbMIDI.read()); // Discard incoming MIDI messages
}

/* Here's the set of MIDI functions for making your own projects:

  usbMIDI.sendNoteOn(note, velocity, channel)
  usbMIDI.sendNoteOff(note, velocity, channel)
  usbMIDI.sendPolyPressure(note, pressure, channel)
  usbMIDI.sendControlChange(control, value, channel)
  usbMIDI.sendProgramChange(program, channel)
  usbMIDI.sendAfterTouch(pressure, channel)
  usbMIDI.sendPitchBend(value, channel)
  usbMIDI.sendSysEx(length, array)
  usbMIDI.send_now()

  Some info on MIDI note numbers can be found here:
  http://www.phys.unsw.edu.au/jw/notes.html

  Rather than MIDI, one could theoretically try using Serial to
  create a sketch compatible with serialosc or monomeserial, but
  those tools have proven notoriously unreliable thus far. */
