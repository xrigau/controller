/* Copyright (C) 2014-2016 by Jacob Alexander
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// ----- Includes -----

// Compiler Includes
#include <stdint.h>

// TODO Needs to be defined per keyboard
#define LED_TotalChannels 144

#define LED_BufferLength 144

// ----- Functions -----

void LED_setup();
uint8_t LED_scan();

void LED_currentChange( unsigned int current );


typedef struct LED_Buffer {
	uint8_t i2c_addr;
	uint8_t reg_addr;
	uint8_t buffer[LED_BufferLength];
} LED_Buffer;

// Custom
LED_Buffer LED_pageBuffer;

void LED_sendPage( uint8_t *buffer, uint8_t len, uint8_t page );