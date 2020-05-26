/*!
 * @file Adafruit_UNTZtrument.h
 *
 * Provides Adafruit_UNTZtrument and enc (encoder) classes.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
 */

#ifndef _UNTZTRUMENT_H_
#define _UNTZTRUMENT_H_

#include <Adafruit_Trellis.h>
#include <Arduino.h>

// UNTZTRUMENT/TRELLIS STUFF -----------------------------------------------

/**
 * @brief  The Adafruit_UNTZtrument class is just a slight wrapper around a
 *         TrellisSet (see the Adafruit_Trellis library for details), adding
 *         X/Y-to-button-index and button-index-to-X/Y functions.
 */
class Adafruit_UNTZtrument : public Adafruit_TrellisSet {
public:
  /**
   * @brief  Construct a new Adafruit_UNTZtrument object.
   * @param  matrix0  First (or only) Adafruit_NeoTrellis object forming
   *                  the overall UNTZtrument grid.
   * @param  matrix1  Second Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix2  Third Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix3  Fourth Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix4  Fifth Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix5  Sixth Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix6  Seventh Adafruit_NeoTrellis (or NULL for none).
   * @param  matrix7  Eighth Adafruit_NeoTrellis (or NULL for none).
   */
  Adafruit_UNTZtrument(Adafruit_Trellis *matrix0, Adafruit_Trellis *matrix1 = 0,
                       Adafruit_Trellis *matrix2 = 0,
                       Adafruit_Trellis *matrix3 = 0,
                       Adafruit_Trellis *matrix4 = 0,
                       Adafruit_Trellis *matrix5 = 0,
                       Adafruit_Trellis *matrix6 = 0,
                       Adafruit_Trellis *matrix7 = 0);

  /**
   * @brief  Convert button/LED at (x,y) position to absolute index.
   * @param  x        Button/LED column.
   * @param  y        Button/LED row.
   * @return uint8_t  Absolute index (0 to total buttons - 1)
   */
  uint8_t xy2i(uint8_t x, uint8_t y);
  /**
   * @brief  Convert button/LED absolute index to (x,y) position.
   * @param  i  Button/LED index.
   * @param  x  Pointer to uint8_t* where column is returned.
   * @param  y  Pointer to uint8_t* where row is returned.
   */
  void i2xy(uint8_t i, uint8_t *x, uint8_t *y);

private:
  uint8_t size;
};

// ENCODER STUFF -----------------------------------------------------------

/** Encoder values & bounds are 16-bit signed; balance between RAM and
    resolution.  Could easily change to 8 or 32 bits if an application
    needs it, but unsigned types will require a change to the default
    min & max values in the constructor (see comment in .cpp). */
typedef int16_t enc_t;

/**
 * @brief  The enc class provides basic encoder support.  It's polling-based
 *         (rather than interrupt-based) so it can work with any pins and any
 *         number of encoders, but the poll() function must be called
 *         frequently (a ~1 ms timer interrupt can optionally be used).
 *         Supports upper/lower limits with clipping or wraparound.
 *         Does not (currently) provide acceleration, detent division or
 *         shaft button debouncing...just basics, can use alternative
 *         libraries if special functionality is required.
 */
class enc {

public:
  /**
   * @brief  Construct a new enc (encoder) object.
   * @param  a  First pin (Arduino pin #).
   * @param  b  Second pin (Arduino pin #).
   * @param  p  Enable pull-up resistors on pins if true (default). Use
   *            this for open-drain encoders such as PEC11 -- tie C pin
   *            to GND.
   */
  enc(uint8_t a, uint8_t b, boolean p = true);

  /**
   * @brief  Set upper/lower limits of the encoders range (inclusive),
   *         and whether or not to wrap-around at ends. Note: if changing
   *         bounds AND value, set bounds first!
   * @param  lo    Encoder value lower limit.
   * @param  hi    Encoder value upper limit.
   * @param  wrap  If true, wrap around when lo/hi are exceeded, otherwise
   *               stop and ends.
   */
  void setBounds(enc_t lo, enc_t hi, boolean wrap = false);

  /**
   * @brief  Set current encoder value, applying any previously-set bounds
   *         limits..
   * @param  v  Encoder value. Will be clipped/wrapped as needed to match
   *            last setBounds() values.
   */
  void setValue(enc_t v);

  /**
   * @brief    Get current encoder value.
   * @returns  enc_t  Encoder value.
   */
  enc_t getValue(void);

  // Static member funcs are shared across all objects, not per-instance.
  // e.g. enc::poll() reads the lot, no need to call for each item.

  /**
   * @brief  Initialize pins for all enc objects. Must call this in
   *         setup() function before using encoders.
   */
  static void begin(void);

  /**
   * @brief  Read all encoder inputs and update encoder values.
   */
  static void poll(void);

private:
  uint8_t pinA, pinB;
  boolean pullup;
#if defined(ARDUINO_ARCH_SAMD)
  volatile uint32_t *pinRegA, *pinRegB; // PIN registers for A & B
  uint32_t pinMaskA, pinMaskB;          // Bitmask for A & B
#else
  volatile uint8_t *pinRegA, *pinRegB; // PIN registers for A & B
  uint8_t pinMaskA, pinMaskB;          // Bitmask for A & B
#endif
  enc_t value,   // Current value
      min, max;  // Range bounds, if set
  uint8_t flags, // Limit (clip) vs wraparound
      state;     // Saved encoder pin state
  int8_t x2;     // Last motion X2 (-2,0,+2)
  enc *next;     // Next in linked list
};

#endif // _UNTZTRUMENT_H_
