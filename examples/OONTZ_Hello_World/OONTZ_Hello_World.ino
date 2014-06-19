// Super-basic OONTZ MIDI example.  Maps 8X8 OONTZ buttons to MIDI
// note on/off events; this is NOT a sequencer or anything fancy.
// Requires an Arduino Leonardo w/TeeOnArdu config (or a PJRC Teensy),
// software on host computer for synth or to route to other devices.

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_OONTZ.h>

#define LED     13 // Pin for heartbeat LED (shows code is working)
#define CHANNEL 1  // MIDI channel number

// The OONTZ config for this example is comprised of four Trellises in
// a 2x2 arrangement (8x8 buttons total).  addr[] is the I2C address
// of the upper left, upper right, lower left and lower right matrices,
// respectively, assuming an upright orientation, i.e. labels on board
// are in the normal reading direction.
Adafruit_Trellis T[4];
OONTZ            oontz(&T[0], &T[1], &T[2], &T[3]);
const uint8_t    addr[] = { 0x70, 0x71, 0x72, 0x73 };
// For a 16x8 OONTZ, we'd instead use something like:
// Adafruit_Trellis T[8];
// OONTZ            oontz(&T[0], &T[1], &T[2], &T[3],
//                        &T[4], &T[5], &T[6], &T[7]);
// const uint8_t    addr[] = { 0x70, 0x71, 0x72, 0x73,
//                             0x74, 0x75, 0x76, 0x77 };
// The oontz.begin() later then requires the extra 4 addr's.

// For this example, MIDI note numbers are simply centered based on
// the number of Trellis buttons; each row doesn't necessarily
// correspond to an octave or anything.
#define WIDTH     ((sizeof(T) / sizeof(T[0])) * 2)
#define N_BUTTONS ((sizeof(T) / sizeof(T[0])) * 16)
#define LOWNOTE   ((127 - N_BUTTONS) / 2)

uint8_t       heart        = 0;  // Heartbeat LED counter
unsigned long prevReadTime = 0L; // Keypad polling timer

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
}

void loop() {
  unsigned long t = millis();
  if((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time
    if(oontz.readSwitches()) {  // Button state change?
      for(uint8_t i=0; i<N_BUTTONS; i++) { // For each button...
        // Get column/row for button, convert to MIDI note number
        uint8_t x, y, note;
        oontz.i2xy(i, &x, &y);
        note = LOWNOTE + y * WIDTH + x;
        if(oontz.justPressed(i)) {
          usbMIDI.sendNoteOn(note, 127, CHANNEL);
          oontz.setLED(i);
        } else if(oontz.justReleased(i)) {
          usbMIDI.sendNoteOff(note, 0, CHANNEL);
          oontz.clrLED(i);
        }
      }
      oontz.writeDisplay();
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
