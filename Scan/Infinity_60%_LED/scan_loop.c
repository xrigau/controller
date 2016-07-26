/* Copyright (C) 2014-2016 by Jacob Alexander
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// ----- Includes -----

// Compiler Includes
#include <Lib/ScanLib.h>

// Project Includes
#include <cli.h>
#include <led.h>
#include <led_scan.h>
#include <print.h>
#include <matrix_scan.h>
#include <macro.h>
#include <output_com.h>

// Local Includes
#include "scan_loop.h"



// ----- Function Declarations -----

// ----- Variables -----

// Number of scans since the last USB send
uint16_t Scan_scanCount = 0;

uint8_t LED_debounce_timer = 0;
uint8_t LED_luminosity = 245;
uint8_t LED_areAllKeysIlluminated = 1;

// ----- Functions -----

// Setup
inline void Scan_setup()
{
	// Setup GPIO pins for matrix scanning
	Matrix_setup();

	// Setup ISSI chip to control the leds
	LED_setup();

	// Reset scan count
	Scan_scanCount = 0;
}


// Main Detection Loop
inline uint8_t Scan_loop()
{
	Matrix_scan( Scan_scanCount++ );

	// Process any LED events
	LED_scan();

	return 0;
}


// Signal from Macro Module that all keys have been processed (that it knows about)
inline void Scan_finishedWithMacro( uint8_t sentKeys )
{
}


// Signal from Output Module that all keys have been processed (that it knows about)
inline void Scan_finishedWithOutput( uint8_t sentKeys )
{
	// Reset scan loop indicator (resets each key debounce state)
	// TODO should this occur after USB send or Macro processing?
	Scan_scanCount = 0;
}


// Signal from the Output Module that the available current has changed
// current - mA
void Scan_currentChange( unsigned int current )
{
	// Indicate to all submodules current change
	Matrix_currentChange( current );
	LED_currentChange( current );
}



// ----- Capabilities -----

// Custom capability examples
// Refer to kll.h in Macros/PartialMap for state and stateType information
void CustomAction_action1_capability( uint8_t state, uint8_t stateType, uint8_t *args )
{
	// Display capability name
	// XXX This is required for debug cli to give you a list of capabilities
	if ( stateType == 0xFF && state == 0xFF )
	{
		print("CustomAction_action1_capability()");
		return;
	}

	// Prints Action1 info message to the debug cli
	info_print("Action1");
}

uint8_t CustomAction_blockHold_storage = 0;
void CustomAction_blockHold_capability( uint8_t state, uint8_t stateType, uint8_t *args )
{
	// Display capability name
	if ( stateType == 0xFF && state == 0xFF )
	{
		print("CustomAction_blockHold_capability(usbCode)");
		return;
	}

	// Retrieve 8-bit argument
	uint8_t key = args[0];

	// We only care about normal keys
	if ( stateType == 0x00 )
	{
		// Block given key if we're in the "Press" or "Hold" state
		if ( ( state == 0x01 || state == 0x02 )
			&& CustomAction_blockHold_storage == 0 )
		{
			CustomAction_blockHold_storage = key;
			info_msg("Blocking Key: ");
			printHex( key );
			print( NL );
		}
		// Release if in the "Off" or "Release" state and we're blocking
		else if ( ( state == 0x00 || state == 0x03 )
			&& key == CustomAction_blockHold_storage )
		{
			info_msg("Unblocking Key: ");
			printHex( CustomAction_blockHold_storage );
			print( NL );
			CustomAction_blockHold_storage = 0;
		}
	}
}

void CustomAction_blockKey_capability( uint8_t state, uint8_t stateType, uint8_t *args )
{
	// Display capability name
	if ( stateType == 0xFF && state == 0xFF )
	{
		print("CustomAction_blockKey_capability(usbCode)");
		return;
	}

	// Retrieve 8-bit argument
	uint8_t key = args[0];

	// If key is not blocked, process
	if ( key != CustomAction_blockHold_storage )
	{
		extern void Output_usbCodeSend_capability( uint8_t state, uint8_t stateType, uint8_t *args );
		Output_usbCodeSend_capability( state, stateType, &key );
	}
}

uint8_t isDebug(uint8_t state, uint8_t stateType) {
	return stateType == 0xFF && state == 0xFF;
}

uint8_t isNotAKeyPress(uint8_t state, uint8_t stateType) {
	return stateType == 0x00 && state != 0x01;
}

void pushLedPage() {
	LED_pageBuffer.i2c_addr = 0xE8; // Chip 1
	LED_pageBuffer.reg_addr = 0x24; // Brightness section
	LED_sendPage( (uint8_t*)&LED_pageBuffer, sizeof( LED_Buffer ), 0 );
}

void clearPageBuffer() {
	for ( uint8_t i = 0; i < LED_TotalChannels; i++ ) {
		LED_pageBuffer.buffer[ i ] = 0;
	}
}

void illuminateFunctionLayerKeys() {
	clearPageBuffer();
	int8_t values[] = {16, 32, 48, 64, 80, 96, 112, 7, 0, 33, 49, 65, 81, 97, 113, 85, 70, 86, 102, 99, 103, 38};
	for (uint8_t i = 0; i < sizeof(values); i++) {
		LED_pageBuffer.buffer[values[i]] = LED_luminosity;
	}
	pushLedPage();
}

void illuminateAllKeys() {
	for ( uint8_t i = 0; i < LED_TotalChannels; i++ ) {
		LED_pageBuffer.buffer[i] = LED_luminosity;
	}
	pushLedPage();
}

void illuminateEscKey() {
	clearPageBuffer();
	LED_pageBuffer.buffer[16] = LED_luminosity;
	pushLedPage();
}

void CustomAction_updateLeds_capability(uint8_t state, uint8_t stateType, uint8_t *args) {
	if (isDebug(state, stateType)) {
		// Display capability name. This is required for debug cli to give you a list of capabilities
		print("CustomAction_updateLeds_capability()");
		return;
	}
	if (isNotAKeyPress(state, stateType)) {
		// Only use capability on press. Not on release
		return;
	}

	if (LED_areAllKeysIlluminated == 1) {
		illuminateFunctionLayerKeys();
		LED_areAllKeysIlluminated = 0;
	} else {
		illuminateAllKeys();
		LED_areAllKeysIlluminated = 1;
	}
}


uint8_t isKeyPress(uint8_t state, uint8_t stateType) {
	return stateType == 0x00 && state == 0x01;
}

uint8_t isKeyRelease(uint8_t state, uint8_t stateType) {
	return stateType == 0x00 && state == 0x03;
}

uint8_t isKeyHold(uint8_t state, uint8_t stateType) {
	return stateType == 0x00 && state == 0x02;
}

void CustomAction_lightEsc_capability(uint8_t state, uint8_t stateType, uint8_t *args) {
	if (isDebug(state, stateType)) {
		// Display capability name. This is required for debug cli to give you a list of capabilities
		print("CustomAction_lightEsc_capability()");
		return;
	}

	if (isKeyPress(state, stateType) == 1) {
		illuminateEscKey();
	} else if (isKeyRelease(state, stateType) == 1) {
		illuminateAllKeys();
	}
}

uint8_t isTooEarlyToUpdate() {
	uint8_t currentTime = (uint8_t)systick_millis_count;
	int8_t compare = (int8_t)(currentTime - LED_debounce_timer) & 0x7F;
	if (compare < 30) {
		return 1;
	} else {
		LED_debounce_timer = currentTime;
		return 0;
	}
}

void propagateNewLuminosity() {
	if (LED_areAllKeysIlluminated == 1) {
		illuminateAllKeys();
	} else {
		illuminateFunctionLayerKeys();
	}
}

void updateLuminosityWithBounds(uint8_t amount) {
	LED_luminosity += amount;
	if (LED_luminosity > 245) {
		LED_luminosity = 245;
	} else if (LED_luminosity < 10) {
		LED_luminosity = 10;
	}
}

void changeLuminosity(uint8_t amount, uint8_t state, uint8_t stateType) {
	if (isDebug(state, stateType)) {
		print("CustomAction_increaseAllLedsLuminosity_capability()");
		print("CustomAction_decreaseAllLedsLuminosity_capability()");
		return;
	}

	if (isTooEarlyToUpdate() == 1) {
		return;
	}

	if (isKeyHold(state, stateType) == 1) {
		updateLuminosityWithBounds(amount);
		propagateNewLuminosity();
	}
}

void CustomAction_increaseAllLedsLuminosity_capability(uint8_t state, uint8_t stateType, uint8_t *args) {
	changeLuminosity(10, state, stateType);
}

void CustomAction_decreaseAllLedsLuminosity_capability(uint8_t state, uint8_t stateType, uint8_t *args) {
	changeLuminosity(-10, state, stateType);
}

void CustomAction_turnAllLedsOff_capability(uint8_t state, uint8_t stateType, uint8_t *args) {
	if (isDebug(state, stateType)) {
		// Display capability name. This is required for debug cli to give you a list of capabilities
		print("CustomAction_turnAllLedsOff_capability()");
		return;
	}

	if (isKeyPress(state, stateType) == 1) {
		LED_luminosity = 0;
		clearPageBuffer();
		pushLedPage();
	}
}

