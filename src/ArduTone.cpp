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

/**
 *
 * The tone_alarm driver supports a set of predefined "alarm"
 * tunes and one user-supplied tune.
 *
 * The TONE_SET_ALARM ioctl can be used to select a predefined
 * alarm tune, from 1 - <TBD>.  Selecting tune zero silences
 * the alarm.
 *
 * Tunes follow the syntax of the Microsoft GWBasic/QBasic PLAY
 * statement, with some exceptions and extensions.
 *
 * From Wikibooks:
 *
 * PLAY "[string expression]"
 *
 * Used to play notes and a score ... The tones are indicated by letters A through G.
 * Accidentals are indicated with a "+" or "#" (for sharp) or "-" (for flat)
 * immediately after the note letter. See this example:
 *
 *   PLAY "C C# C C#"
 *
 * Whitespaces are ignored inside the string expression. There are also codes that
 * set the duration, octave and tempo. They are all case-insensitive. PLAY executes
 * the commands or notes the order in which they appear in the string. Any indicators
 * that change the properties are effective for the notes following that indicator.
 *
 * Ln     Sets the duration (length) of the notes. The variable n does not indicate an actual duration
 *        amount but rather a note type; L1 - whole note, L2 - half note, L4 - quarter note, etc.
 *        (L8, L16, L32, L64, ...). By default, n = 4.
 *        For triplets and quintets, use L3, L6, L12, ... and L5, L10, L20, ... series respectively.
 *        The shorthand notation of length is also provided for a note. For example, "L4 CDE L8 FG L4 AB"
 *        can be shortened to "L4 CDE F8G8 AB". F and G play as eighth notes while others play as quarter notes.
 * On     Sets the current octave. Valid values for n are 0 through 6. An octave begins with C and ends with B.
 *        Remember that C- is equivalent to B.
 * < >    Changes the current octave respectively down or up one level.
 * Nn     Plays a specified note in the seven-octave range. Valid values are from 0 to 84. (0 is a pause.)
 *        Cannot use with sharp and flat. Cannot use with the shorthand notation neither.
 * MN     Stand for Music Normal. Note duration is 7/8ths of the length indicated by Ln. It is the default mode.
 * ML     Stand for Music Legato. Note duration is full length of that indicated by Ln.
 * MS     Stand for Music Staccato. Note duration is 3/4ths of the length indicated by Ln.
 * Pn     Causes a silence (pause) for the length of note indicated (same as Ln).
 * Tn     Sets the number of "L4"s per minute (tempo). Valid values are from 32 to 255. The default value is T120.
 * .      When placed after a note, it causes the duration of the note to be 3/2 of the set duration.
 *        This is how to get "dotted" notes. "L4 C#." would play C sharp as a dotted quarter note.
 *        It can be used for a pause as well.
 *
 * Extensions/variations:
 *
 * MB MF  The MF command causes the tune to play once and then stop. The MB command causes the
 *        tune to repeat when it ends.
 *
 */

#include "ArduTone.h"
#define debug(s) Serial.println(s)
static const uint8_t ArduTone::_note_tab[7] = {9, 11, 0, 2, 4, 5, 7};
ArduTone::ArduTone(uint8_t pin,char *arg_tune){
	tonePin = pin;
	_tune = arg_tune;
	pinMode(tonePin,OUTPUT);
	debug("ArduTone Init");
}
uint32_t ArduTone::note_duration(uint32_t &silence, uint32_t note_length, uint32_t dots)
{
	uint32_t whole_note_period = (60 * 1000000 * 4) / _tempo;
	
	if (note_length == 0) {
		note_length = 1;
	}

	uint32_t note_period = whole_note_period / note_length;
debug("--");
	debug(_tempo);
	debug(whole_note_period);
	debug(note_period);
	debug("--");
	switch (_note_mode) {
	case MODE_NORMAL:
		silence = note_period / 8;
		break;

	case MODE_STACCATO:
		silence = note_period / 4;
		break;

	default:
	case MODE_LEGATO:
		silence = 0;
		break;
	}

	note_period -= silence;

	uint32_t dot_extension = note_period / 2;

	while (dots--) {
		note_period += dot_extension;
		dot_extension /= 2;
	}

	return note_period;
}

uint32_t ArduTone::rest_duration(uint32_t rest_length, uint32_t dots)
{
	uint32_t whole_note_period = (60 * 1000000 * 4) / _tempo;

	if (rest_length == 0) {
		rest_length = 1;
	}

	uint32_t rest_period = whole_note_period / rest_length;

	uint32_t dot_extension = rest_period / 2;

	while (dots--) {
		rest_period += dot_extension;
		dot_extension /= 2;
	}

	return rest_period;
}

void
ArduTone::start_note(uint32_t note)
{
	debug("Playing note:");
	debug(note);
	// compute the frequency first (Hz)
	double freq = 880.0f * expf(logf(2.0f) * ((int)note - 46) / 12.0f);
	debug(freq);
	tone(tonePin,freq);
}

void ArduTone::stop_note()
{
	debug("Stop playing note");
	noTone(tonePin);
}
void ArduTone::call_after(uint32_t time){
	debug("call after:");
	debug(time);
	if(time){
		delay(time/(uint32_t)(1000*2));
	}
	next_note();
	return;
}
void ArduTone::start()
{
	debug("start:");
	debug(_tune);
	// stop any current playback
	stop_note();

	// record the tune
	_next = _tune;

	// initialise player state
	_tempo = 120;
	_note_length = 4;
	_note_mode = MODE_NORMAL;
	_octave = 4;
	_silence_length = 0;
	repeat = false;
	
	// schedule a callback to start playing
	call_after(0);
}

void
ArduTone::next_note()
{
	debug("next_note");
	// do we have an inter-note gap to wait for?
	if (_silence_length > 0) {
		debug("Wait for silence length");
		stop_note();
		uint32_t _silence_length_val = _silence_length;
		_silence_length = 0;
		call_after(_silence_length_val);
		return;
	}

	// make sure we still have a tune - may be removed by the write / ioctl handler
	if ((_next == nullptr) || (_tune == nullptr)) {
		debug("No tune available");
		stop_note();
		return;
	}

	// parse characters out of the string until we have resolved a note
	uint32_t note = 0;
	uint32_t note_length = _note_length;
	uint32_t duration;

	while (note == 0) {
		// we always need at least one character from the string
		int c = next_char();
		debug("next_char:");
		debug((char)c);
		if (c == 0) {
			goto tune_end;
		}

		_next++;

		switch (c) {
		case 'L':	// select note length
			_note_length = next_number();

			if (_note_length < 1) {
				goto tune_error;
			}

			break;

		case 'O':	// select octave
			_octave = next_number();

			if (_octave > 6) {
				_octave = 6;
			}

			break;

		case '<':	// decrease octave
			if (_octave > 0) {
				_octave--;
			}

			break;

		case '>':	// increase octave
			if (_octave < 6) {
				_octave++;
			}

			break;

		case 'M':	// select inter-note gap
			c = next_char();

			if (c == 0) {
				goto tune_error;
			}

			_next++;

			switch (c) {
			case 'N':
				_note_mode = MODE_NORMAL;
				break;

			case 'L':
				_note_mode = MODE_LEGATO;
				break;

			case 'S':
				_note_mode = MODE_STACCATO;
				break;

			case 'F':
				repeat = false;
				break;

			case 'B':
				repeat = true;
				break;

			default:
				goto tune_error;
			}

			break;

		case 'P':	// pause for a note length
			stop_note();
			call_after(rest_duration(next_number(), next_dots()));
			return;

		case 'T': {	// change tempo
				uint32_t nt = next_number();

				if ((nt >= 32) && (nt <= 255)) {
					_tempo = nt;

				} else {
					goto tune_error;
				}

				break;
			}

		case 'N':	// play an arbitrary note
			note = next_number();

			if (note > 84) {
				goto tune_error;
			}

			if (note == 0) {
				// this is a rest - pause for the current note length
				call_after(rest_duration(_note_length, next_dots()));
				return;
			}

			break;

		case 'A'...'G':	// play a note in the current octave
			note = _note_tab[c - 'A'] + (_octave * 12) + 1;
			c = next_char();

			switch (c) {
			case '#':	// up a semitone
			case '+':
				if (note < 84) {
					note++;
				}

				_next++;
				break;

			case '-':	// down a semitone
				if (note > 1) {
					note--;
				}

				_next++;
				break;

			default:
				// 0 / no next char here is OK
				break;
			}

			// shorthand length notation
			note_length = next_number();

			if (note_length == 0) {
				note_length = _note_length;
			}

			break;

		default:
			goto tune_error;
		}
	}

	// compute the duration of the note and the following silence (if any)
	duration = note_duration(_silence_length, note_length, next_dots());

	// start playing the note
	start_note(note);

	// and arrange a callback when the note should stop
	call_after(duration);
	return;

	// tune looks bad (unexpected EOF, bad character, etc.)
tune_error:
	debug("tune error\n");
	repeat = false;		// don't loop on error

	// stop (and potentially restart) the tune
tune_end:
	stop_note();

	if (repeat) {
		start();
	} else {
		//_tune = nullptr; //do not make tune null
	}

	return;
}

int ArduTone::next_char()
{
	while (isspace(*_next)) {
		_next++;
	}
	
	return toupper(*_next);
}

uint32_t ArduTone::next_number()
{
	uint32_t number = 0;
	int c;

	for (;;) {
		c = next_char();

		if (!isdigit(c)) {
			return number;
		}

		_next++;
		number = (number * 10) + (c - '0');
	}
}

uint32_t ArduTone::next_dots()
{
	uint32_t dots = 0;

	while (next_char() == '.') {
		_next++;
		dots++;
	}

	return dots;
}
