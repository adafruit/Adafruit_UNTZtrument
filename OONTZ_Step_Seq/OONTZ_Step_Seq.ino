// Simple Trellis MIDI step sequencer.
// Requires a PJRC Teensy or an Arduino Leonardo w/TeeOnArdu config,
// software on host computer for synth or to route to other devices.

#include <Wire.h>
#include <Adafruit_Trellis.h>

#define LED 13 // Pin for heartbeat LED (shows code is working)

// The TrellisSet for this example is comprised of four Trellises in
// a 2x2 arrangement (8x8 buttons total).  addr[] is the I2C address
// of the upper left, upper right, lower left and lower right matrices,
// respectively, assuming an upright orientation, i.e. labels on board
// are in the normal reading direction.
Adafruit_Trellis    T[4];
Adafruit_TrellisSet trellis(&T[0], &T[1], &T[2], &T[3]);
const uint8_t       addr[] = { 0x70, 0x71, 0x72, 0x73 };

uint8_t       grid[8];                     // Sequencer state
uint8_t       heart        = 0,            // Heartbeat LED counter
              col          = 0;            // Current column
unsigned int  bpm          = 240;          // Tempo
unsigned long beatInterval = 60000L / bpm, // ms/beat
              prevBeatTime = 0L,           // Column step timer
              prevReadTime = 0L;           // Keypad polling timer

// The first two tables here simplify writing sequencers and instruments.
// i2xy converts a button index (0-63) to an X/Y pair (high 4 bits are
// column, low 4 are row).  xy2i is the opposite, remapping a column (0-7)
// and row (0-7) to a button or LED index.  Both reside in flash space; to
// access, use xy=pgm_read_byte(&i2xy[i]) or i=pgm_read_byte(&xy2i[x][y]).
// The note[] and channel[] tables are the MIDI note and channel numbers
// for to each row (top to bottom); they're specific to this application.
// bitmask[] is for efficient reading/writing bits to the grid[] array.
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
    { 19, 23, 27, 31, 51, 55, 59, 63 } },
  note[8]    = {  72, 71, 69, 67, 65, 64, 62, 60 },
  channel[8] = {   1,  1,  1,  1,  1,  1,  1,  1 },
  bitmask[8] = { 128, 64, 32, 16,  8,  4,  2,  1 };

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
  memset(grid, 0, sizeof(grid));
}

// Turn on (or off) one column of the display
void line(uint8_t x, boolean set) {
  uint8_t mask = pgm_read_byte(&bitmask[x]);
  for(uint8_t y=0; y<8; y++) {
    uint8_t i = pgm_read_byte(&xy2i[x][y]);
    if(set || (grid[y] & mask)) trellis.setLED(i);
    else                        trellis.clrLED(i);
  }
}

void loop() {
  uint8_t       mask;
  boolean       refresh = false;
  unsigned long t       = millis();

  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(trellis.readSwitches()) {  // Button state change?
      for(uint8_t i=0; i<sizeof(i2xy); i++) { // For each button...
        uint8_t xy = pgm_read_byte(&i2xy[i]),
                x  = xy >> 4,
                y  = xy & 0x0F;
        mask = pgm_read_byte(&bitmask[x]);
        if(trellis.justPressed(i)) {
          if(grid[y] & mask) { // Already set?  Turn off...
            grid[y] &= ~mask;
            trellis.clrLED(i);
            usbMIDI.sendNoteOff(pgm_read_byte(&note[y]),
              127, pgm_read_byte(&channel[y]));
          } else {         // Turn on
            grid[y] |= mask;
            trellis.setLED(i);
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
    mask = pgm_read_byte(&bitmask[col]);
    for(uint8_t row=0; row<8; row++) {
      if(grid[row] & mask) {
        usbMIDI.sendNoteOff(pgm_read_byte(&note[row]), 127,
          pgm_read_byte(&channel[row]));
      }
    }
    // Advance column counter, wrap around
    if(++col > 7) col = 0;
    // Turn on new column
    line(col, true);
    mask = pgm_read_byte(&bitmask[col]);
    for(uint8_t row=0; row<8; row++) {
      if(grid[row] & mask) {
        usbMIDI.sendNoteOn(pgm_read_byte(&note[row]), 127,
          pgm_read_byte(&channel[row]));
      }
    }
    prevBeatTime = t;
    refresh      = true;
  }

  if(refresh) trellis.writeDisplay();

  while(usbMIDI.read()); // Discard incoming MIDI messages
}
