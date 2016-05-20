/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/
#ifndef _hal_hal_h_
#define _hal_hal_h_

#ifdef ARDUINO

static const int NUM_DIO = 3;

struct lmic_pinmap {
    u1_t nss;
    u1_t rxtx;
    u1_t rst;
    u1_t dio[NUM_DIO];
};

#else
uint64_t clock_ms();
#define millis clock_ms
#define micros us_ticker_read

static const int NUM_DIO = 5;

struct lmic_pinmap {
    DigitalOut nss;
    DigitalOut rxtx;
    DigitalInOut rst;
    DigitalInOut dio[NUM_DIO];
};
#endif

// Declared here, to be defined an initialized by the application
extern lmic_pinmap pins;

#endif // _hal_hal_h_
