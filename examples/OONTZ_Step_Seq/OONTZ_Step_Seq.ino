// Simple OONTZ MIDI step sequencer.
// Requires an Arduino Leonardo w/TeeOnArdu config (or a PJRC Teensy),
// software on host computer for synth or to route to other devices.

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_OONTZ.h>

#define LED 13 // Pin for heartbeat LED (shows code is working)

// The OONTZ config for this example is comprised of four Trellises in
// a 2x2 arrangement (8x8 buttons total).  addr[] is the I2C address
// of the upper left, upper right, lower left and lower right matrices,
// respectively, assuming an upright orientation, i.e. labels on board
// are in the normal reading direction.
Adafruit_Trellis    T[4];
OONTZ               oontz(&T[0], &T[1], &T[2], &T[3]);
const uint8_t       addr[] = { 0x70, 0x71, 0x72, 0x73 };
// For a 16x8 OONTZ, we'd instead use something like:
// Adafruit_Trellis T[8];
// OONTZ            oontz(&T[0], &T[1], &T[2], &T[3],
//                        &T[4], &T[5], &T[6], &T[7]);
// const uint8_t    addr[] = { 0x70, 0x71, 0x72, 0x73,
//                             0x74, 0x75, 0x76, 0x77 };
// The oontz.begin() later then requires the extra 4 addr's.

#define WIDTH     ((sizeof(T) / sizeof(T[0])) * 2)
#define N_BUTTONS ((sizeof(T) / sizeof(T[0])) * 16)

uint8_t       grid[WIDTH];                 // Sequencer state
uint8_t       heart        = 0,            // Heartbeat LED counter
              col          = WIDTH-1;      // Current column
unsigned int  bpm          = 240;          // Tempo
unsigned long beatInterval = 60000L / bpm, // ms/beat
              prevBeatTime = 0L,           // Column step timer
              prevReadTime = 0L;           // Keypad polling timer

// The note[] and channel[] tables are the MIDI note and channel numbers
// for to each row (top to bottom); they're specific to this application.
// bitmask[] is for efficient reading/writing bits to the grid[] array.
static const uint8_t PROGMEM
  note[8]    = {  72, 71, 69, 67, 65, 64, 62,  60 },
  channel[8] = {   1,  1,  1,  1,  1,  1,  1,   1 },
  bitmask[8] = {   1,  2,  4,  8, 16, 32, 64, 128 };

void setup() {
  pinMode(LED, OUTPUT);
  oontz.begin(addr[0], addr[1], addr[2], addr[3]);
#ifdef __AVR__
  // Default Arduino I2C speed is 100 KHz, but the HT16K33 supports
  // 400 KHz.  We can force this for faster read & refresh, but may
  // break compatibility with other I2C devices...so be prepared to
  // comment this out, or save & restore value as needed.
  TWBR = 12;
#endif
  oontz.clear();
  oontz.writeDisplay();
  memset(grid, 0, sizeof(grid));
}

// Turn on (or off) one column of the display
void line(uint8_t x, boolean set) {
  for(uint8_t mask=1, y=0; y<8; y++, mask <<= 1) {
    uint8_t i = oontz.xy2i(x, y);
    if(set || (grid[x] & mask)) oontz.setLED(i);
    else                        oontz.clrLED(i);
  }
}

void loop() {
  uint8_t       mask;
  boolean       refresh = false;
  unsigned long t       = millis();

  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(oontz.readSwitches()) {    // Button state change?
      for(uint8_t i=0; i<N_BUTTONS; i++) { // For each button...
        uint8_t x, y;
        oontz.i2xy(i, &x, &y);
        mask = pgm_read_byte(&bitmask[y]);
        if(oontz.justPressed(i)) {
          if(grid[x] & mask) { // Already set?  Turn off...
            grid[x] &= ~mask;
            oontz.clrLED(i);
            usbMIDI.sendNoteOff(pgm_read_byte(&note[y]),
              127, pgm_read_byte(&channel[y]));
          } else { // Turn on
            grid[x] |= mask;
            oontz.setLED(i);
          }
          refresh = true;
        }
      }
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); // Blink = alive
  }

  if((t - prevBeatTime) >= beatInterval) { // Next beat?
    // Turn off old column
    line(col, false);
    for(uint8_t row=0, mask=1; row<8; row++, mask <<= 1) {
      if(grid[col] & mask) {
        usbMIDI.sendNoteOff(pgm_read_byte(&note[row]), 127,
          pgm_read_byte(&channel[row]));
      }
    }
    // Advance column counter, wrap around
    if(++col >= WIDTH) col = 0;
    // Turn on new column
    line(col, true);
    for(uint8_t row=0, mask=1; row<8; row++, mask <<= 1) {
      if(grid[col] & mask) {
        usbMIDI.sendNoteOn(pgm_read_byte(&note[row]), 127,
          pgm_read_byte(&channel[row]));
      }
    }
    prevBeatTime = t;
    refresh      = true;
  }

  if(refresh) oontz.writeDisplay();

  while(usbMIDI.read()); // Discard incoming MIDI messages
}
