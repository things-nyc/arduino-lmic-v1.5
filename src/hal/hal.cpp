/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman, modifications Thomas Telkamp
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/

#ifdef ARDUINO
#include <Arduino.h>
#include <SPI.h>
#else
#include "mbed.h"
#endif
#include "../lmic.h"
#include "hal.h"

// -----------------------------------------------------------------------------
// I/O

#ifdef ARDUINO
static void hal_io_init () {
    pinMode(pins.nss, OUTPUT);
    pinMode(pins.rxtx, OUTPUT);
    pinMode(pins.rst, OUTPUT);
    pinMode(pins.dio[0], INPUT);
    pinMode(pins.dio[1], INPUT);
    pinMode(pins.dio[2], INPUT);
}
#else
uint64_t clock_ms() { return us_ticker_read() / 1000; }

#ifdef TARGET_NUCLEO_F411RE
SPI device(SPI_MOSI, SPI_MISO, SPI_SCK); 

lmic_pinmap pins {
  /*pins.nss*/ DigitalOut(PB_6),
    /*pins.rxtx*/ DigitalOut(PC_1),
    /*pins.rst*/ DigitalInOut(PA_0, PIN_INPUT, PullNone, 1),
    {
      DigitalInOut(PA_10, PIN_INPUT, PullUp, 1),
	DigitalInOut(PB_3, PIN_INPUT, PullUp, 1),
	DigitalInOut(PB_5, PIN_INPUT, PullUp, 1),
	DigitalInOut(PB_4, PIN_INPUT, PullUp, 1),
	DigitalInOut(PA_9, PIN_INPUT, PullUp, 1),
	//DigitalInOut(PC_7, PIN_INPUT, PullUp, 1),
	}
};
#elif defined(TARGET_KL25Z)
SPI device(PTD2, PTD3, PTD1); 

lmic_pinmap pins {
  /*pins.nss =  */DigitalOut(PTD0),
    /*pins.rxtx = */DigitalOut(PTC2),
    /*pins.rst = */DigitalInOut(PTB0, PIN_INPUT, PullNone, 1),
    {
      DigitalInOut(PTD4, PIN_INPUT, PullUp, 1),
	DigitalInOut(PTA12, PIN_INPUT, PullUp, 1),
	DigitalInOut(PTA4, PIN_INPUT, PullUp, 1),
	DigitalInOut(PTA5, PIN_INPUT, PullUp, 1),
	DigitalInOut(PTA13, PIN_INPUT, PullUp, 1),
	//DigitalInOut(PTD5, PIN_INPUT, PullUp, 1),
	}
};
#else
#error TARGET MUST BE DEFINED TO SET PINS
#endif
static void hal_io_init () {}
#endif

// val == 1  => tx 1
void hal_pin_rxtx (u1_t val) {
#ifdef ARDUINO
  digitalWrite(pins.rxtx, val);
#else
  pins.rxtx = val;
#endif
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (u1_t val) {
    if(val == 0 || val == 1) { // drive pin
#ifdef ARDUINO
      pinMode(pins.rst, OUTPUT);
      digitalWrite(pins.rst, val);
#else
      pins.rst.output();
      pins.rst = val;
#endif
    } else { // keep pin floating
#ifdef ARDUINO
      pinMode(pins.rst, INPUT);
#else
      pins.rst.input();
#endif
    }
}

static bool dio_states[NUM_DIO] = {0};

static void hal_io_check() {
    uint8_t i;
    for (i = 0; i < NUM_DIO; ++i) {
#ifdef ARDUINO
        if (dio_states[i] != digitalRead(pins.dio[i])) {
#else
        if (dio_states[i] != pins.dio[i]) {
#endif
            dio_states[i] = !dio_states[i];
            if (dio_states[i])
                radio_irq_handler(i);
        }
    }
}

// -----------------------------------------------------------------------------
// SPI

#ifdef ARDUINO
static const SPISettings settings(10E6, MSBFIRST, SPI_MODE0);
#endif

static void hal_spi_init () {
#ifdef ARDUINO
    SPI.begin();
#endif
}

void hal_pin_nss (u1_t val) {
#ifdef ARDUINO
    if (!val)
        SPI.beginTransaction(settings);
    else
        SPI.endTransaction();
    //Serial.println(val?">>":"<<");
    digitalWrite(pins.nss, val);
#else
	pins.nss = val;
#endif
}

// perform SPI transaction with radio
u1_t hal_spi (u1_t out) {
#ifdef ARDUINO
   u1_t res = SPI.transfer(out);
#else
  u1_t res = device.write(out);
#endif
/*
    Serial.print(">");
    Serial.print(out, HEX);
    Serial.print("<");
    Serial.println(res, HEX);
    */
    return res;
}

// -----------------------------------------------------------------------------
// TIME

// Keep track of overflow of micros()
u4_t lastmicros=0;
u8_t addticks=0;

static void hal_time_init () {
    lastmicros=0;
    addticks=0;
}

u8_t hal_ticks () {
    // LMIC requires ticks to be 15.5μs - 100 μs long
    // Check for overflow of micros()
    if ( micros()  < lastmicros ) {
        addticks += (u8_t)4294967296 / US_PER_OSTICK;
    }
    lastmicros = micros();
    return ((u8_t)micros() / US_PER_OSTICK) + addticks;
}

// Returns the number of ticks until time. 
static u4_t delta_time(u8_t time) {
      u8_t t = hal_ticks( );
      s4_t d = time - t;
      if (d<=1) { return 0; }
      else {
        return (u4_t)(time - hal_ticks());
      }
}

void hal_waitUntil (u8_t time) {
#ifdef ARDUINO
    u4_t delta = delta_time(time);
    // From delayMicroseconds docs: Currently, the largest value that
    // will produce an accurate delay is 16383.
    while (delta > (16000 / US_PER_OSTICK)) {
        delay(16);
        delta -= (16000 / US_PER_OSTICK);
    }
    if (delta > 0)
        delayMicroseconds(delta * US_PER_OSTICK);
#else
	fprintf(stderr, "Waiting until %u\n", time);
	while(micros() < (((uint64_t) time) * US_PER_OSTICK));
#endif
}

// check and rewind for target time
u1_t hal_checkTimer (u8_t time) {
    // No need to schedule wakeup, since we're not sleeping
    return delta_time(time) <= 0;
}

static uint8_t irqlevel = 0;

void hal_disableIRQs () {
#ifdef ARDUINO
	noInterrupts();
#else
	__disable_irq();
#endif
	irqlevel++;
}

void hal_enableIRQs () {
    if(--irqlevel == 0) {
#ifdef ARDUINO
	interrupts();
#else
        __enable_irq();
#endif
        // Instead of using proper interrupts (which are a bit tricky
        // and/or not available on all pins on AVR), just poll the pin
        // values. Since os_runloop disables and re-enables interrupts,
        // putting this here makes sure we check at least once every
        // loop.
        //
        // As an additional bonus, this prevents the can of worms that
        // we would otherwise get for running SPI transfers inside ISRs
        hal_io_check();
    }
}

void hal_sleep () {
    // Not implemented
}

// -----------------------------------------------------------------------------

void hal_init () {
    // configure radio I/O and interrupt handler
    hal_io_init();
    // configure radio SPI
    hal_spi_init();
    // configure timer and interrupt handler
    hal_time_init();
}

void hal_failed (const char *file, u2_t line) {
#ifdef ARDUINO
    Serial.println("FAILURE");
    Serial.print(file);
    Serial.print(':');
    Serial.println(line);
    Serial.flush();
#endif
	fprintf(stderr, "FAILURE %s:%u\n", file, line);
    hal_disableIRQs();
    while(1);
}

void debug(u4_t n) {
#ifdef ARDUINO
	Serial.println(n); Serial.flush();
#else
	fprintf(stderr,"%u\n", n);
#endif
}

void debug_str(const char *s) {
#ifdef ARDUINO
	Serial.println(s); Serial.flush();
#else
	fprintf(stderr, "%s\n", s);
#endif
}
