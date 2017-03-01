/****************************************************************************
 *
 *   Copyright (C) 2013, 2016 PX4 Development Team. All rights reserved.
 *   Copyright (C) 2017 Tony Lian. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

class ArduTone
{
public:
	ArduTone(uint8_t pin,char *argTrampoline);
	// Start playing
	//
	void start();
	bool repeat;	// if true, tune restarts at end
	enum {
		CBRK_OFF = 0,
		CBRK_ON,
		CBRK_UNINIT
	};

private:
	uint8_t tonePin;
	// semitone offsets from C for the characters 'A'-'G'
	static const uint8_t _note_tab[7];
	const char		*_tune;		// current tune string
	const char		*_next;		// next note in the string
	
	uint32_t		_tempo;
	uint32_t		_note_length;
	enum { MODE_NORMAL, MODE_LEGATO, MODE_STACCATO} _note_mode;
	uint32_t		_octave;
	uint32_t		_silence_length; // if nonzero, silence before next note
	

	
	void 		call_after(uint32_t time);
	
	// Calculate the duration in microseconds of play and silence for a
	// note given the current tempo, length and mode and the number of
	// dots following in the play string.
	//
	uint32_t		note_duration(uint32_t &silence, uint32_t note_length, uint32_t dots);

	// Calculate the duration in microseconds of a rest corresponding to
	// a given note length.
	//
	uint32_t		rest_duration(uint32_t rest_length, uint32_t dots);

	// Start playing the note
	//
	void			start_note(uint32_t note);

	// Stop playing the current note and make the player 'safe'
	//
	void			stop_note();

	// Parse the next note out of the string and play it
	//
	void			next_note();

	// Find the next character in the string, discard any whitespace and
	// return the canonical (uppercase) version.
	//
	int			next_char();

	// Extract a number from the string, consuming all the digit characters.
	//
	uint32_t		next_number();

	// Consume dot characters from the string, returning the number consumed.
	//
	uint32_t		next_dots();

};