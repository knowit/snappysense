// Compiles a melody in the syntax below into a simple C data structure.
//
// The input is a single line on stdin.
// See samples.txt for some examples of the input.
//
// The output is some code on stdout.  The output assumes the existence of these
// type definitions:
//    struct tone { uint16_t frequency; uint16_t duration_ms; };
//    struct music { unsigned length; const struct tone* tones; };
// and defines a const variable `melody` of type music_t with an initializer.
//
// Syntax:
//  Melody ::= Preamble Defaults Notes?
//  Preamble ::= /[^:]*:/
//  Defaults ::= /d=(\d*),o=(\d),b=(\d*):/
//     where \1 is the duration in fractions of a whole note (default 4)
//           \2 is the octave (default 6 but this is immaterial)
//           \3 is the beats per minute (default 63)
//  Notes ::= Note ("," Note)*
//  Note ::= /(\d+)?([cdefgabp])(#)?(\.)?(\d)?/
//     where \1 is the duration in fractions of a whole note (default 4)
//           \2 is the note
//           \3 is the optional # mark
//           \4 is the optional duration lengthening mark
//           \5 is the optional octave (default 6)

package main

import (
	"bufio"
	"fmt"
	"log"
	"math/rand"
	"os"
	"strconv"
)

const (
	RTTTL_OCTAVE_OFFSET = 0
) // offset octave, as needed

const (
	PAUSE    = 0
	NOTE_B0  = 33
	NOTE_CS1 = 35
	NOTE_D1  = 37
	NOTE_DS1 = 39
	NOTE_E1  = 41
	NOTE_F1  = 44
	NOTE_FS1 = 46
	NOTE_G1  = 49
	NOTE_GS1 = 52
	NOTE_A1  = 55
	NOTE_AS1 = 58
	NOTE_B1  = 62
	NOTE_C2  = 65
	NOTE_CS2 = 69
	NOTE_D2  = 73
	NOTE_DS2 = 78
	NOTE_E2  = 82
	NOTE_F2  = 87
	NOTE_FS2 = 93
	NOTE_G2  = 98
	NOTE_GS2 = 104
	NOTE_A2  = 110
	NOTE_AS2 = 117
	NOTE_B2  = 123
	NOTE_C3  = 131
	NOTE_CS3 = 139
	NOTE_D3  = 147
	NOTE_DS3 = 156
	NOTE_E3  = 165
	NOTE_F3  = 175
	NOTE_FS3 = 185
	NOTE_G3  = 196
	NOTE_GS3 = 208
	NOTE_A3  = 220
	NOTE_AS3 = 233
	NOTE_B3  = 247
	NOTE_C4  = 262
	NOTE_CS4 = 277
	NOTE_D4  = 294
	NOTE_DS4 = 311
	NOTE_E4  = 330
	NOTE_F4  = 349
	NOTE_FS4 = 370
	NOTE_G4  = 392
	NOTE_GS4 = 415
	NOTE_A4  = 440
	NOTE_AS4 = 466
	NOTE_B4  = 494
	NOTE_C5  = 523
	NOTE_CS5 = 554
	NOTE_D5  = 587
	NOTE_DS5 = 622
	NOTE_E5  = 659
	NOTE_F5  = 698
	NOTE_FS5 = 740
	NOTE_G5  = 784
	NOTE_GS5 = 831
	NOTE_A5  = 880
	NOTE_AS5 = 932
	NOTE_B5  = 988
	NOTE_C6  = 1047
	NOTE_CS6 = 1109
	NOTE_D6  = 1175
	NOTE_DS6 = 1245
	NOTE_E6  = 1319
	NOTE_F6  = 1397
	NOTE_FS6 = 1480
	NOTE_G6  = 1568
	NOTE_GS6 = 1661
	NOTE_A6  = 1760
	NOTE_AS6 = 1865
	NOTE_B6  = 1976
	NOTE_C7  = 2093
	NOTE_CS7 = 2217
	NOTE_D7  = 2349
	NOTE_DS7 = 2489
	NOTE_E7  = 2637
	NOTE_F7  = 2794
	NOTE_FS7 = 2960
	NOTE_G7  = 3136
	NOTE_GS7 = 3322
	NOTE_A7  = 3520
	NOTE_AS7 = 3729
	NOTE_B7  = 3951
	NOTE_C8  = 4186
	NOTE_CS8 = 4435
	NOTE_D8  = 4699
	NOTE_DS8 = 4978
)

var notes = []int{PAUSE, NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4,
	NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5, NOTE_CS5,
	NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5,
	NOTE_AS5, NOTE_B5, NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6,
	NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7,
	NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7,
	NOTE_AS7, NOTE_B7, NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8}

// Read a line of text from stdin, write output to stdout, errors on stderr
func main() {
	reader := bufio.NewReader(os.Stdin)
	text, _ := reader.ReadString('\n')
	tune := []byte(text)
	idx := 0
	lim := len(tune)
	{
		num := 0

		// load up some safe defaults
		default_dur := 4
		default_oct := 6
		bpm := 63
		tune_name := "(no name)"

		// Skip the preamble
		name_start := idx
		for idx < lim && tune[idx] != ':' {
			idx++ // ignore name
		}
		if idx == lim {
			goto borked
		}
		if idx > name_start {
			tune_name = string(tune[name_start:idx])
		}
		idx++ // skip ':'

		// Parse the defaults
		// get duration
		if idx+1 >= lim || tune[idx] != 'd' || tune[idx+1] != '=' {
			goto borked
		}
		idx += 2
		num, idx = scan_int(tune, idx)
		if num > 0 {
			default_dur = num
		}
		if idx >= lim || tune[idx] != ',' {
			goto borked
		}
		idx++

		// get octave
		if idx+1 >= lim || tune[idx] != 'o' || tune[idx+1] != '=' {
			goto borked
		}
		idx += 2
		if idx >= lim || !isdigit(tune[idx]) {
			goto borked
		}
		num = digval(tune[idx])
		idx++
		if num >= 3 && num <= 7 {
			default_oct = num
		}
		if idx >= lim || tune[idx] != ',' {
			goto borked
		}
		idx++

		// get BPM
		if idx+1 >= lim || tune[idx] != 'b' || tune[idx+1] != '=' {
			goto borked
		}
		idx += 2
		num, idx = scan_int(tune, idx)
		if bpm != 0 {
			bpm = num
		}
		if idx >= lim || tune[idx] != ':' {
			goto borked
		}
		idx++

		// BPM usually expresses the number of quarter notes per minute
		// calculate time for whole note (in milliseconds)
		wholenote := (60 * 1000 / bpm) << 2

		type Tone struct {
			frequency   int
			duration_ms int
		}
		music := make([]Tone, 0)

		for {
			if idx == lim {
				break
			}

			// calculate note duration
			num, idx = scan_int(tune, idx)
			if num == 0 {
				num = default_dur
			}
			duration_ms := wholenote / num

			if idx >= lim {
				goto borked
			}
			c := tune[idx]
			idx++

			note := 0
			scale := 0
			switch c {
			case 'c':
				note = 1
			case 'd':
				note = 3
			case 'e':
				note = 5
			case 'f':
				note = 6
			case 'g':
				note = 8
			case 'a':
				note = 10
			case 'b':
				note = 12
			case 'p':
				note = 14
			}

			// check for optional '#' sharp
			if idx < lim && tune[idx] == '#' {
				note++
				idx++
			}

			// check for optional '.' dotted note
			if idx < lim && tune[idx] == '.' {
				duration_ms += (duration_ms >> 1)
				idx++
			}

			// get scale
			if idx < lim && isdigit(tune[idx]) {
				scale = digval(tune[idx])
				idx++
			} else {
				scale = default_oct
			}
			scale += RTTTL_OCTAVE_OFFSET

			if idx < lim && tune[idx] == ',' {
				idx++ // skip comma for next note
			}

			var frequency = 440
			if note > 0 {
				frequency = notes[(scale-4)*12+note]
			}

			music = append(music, Tone{frequency, duration_ms})
		}

		fmt.Printf("/* Generated by the music-compiler program (see repo root) */\n")
		fmt.Printf("/* %s */\n", tune_name)
		notes_name := "notes_" + strconv.Itoa(rand.Int())
		fmt.Printf("const struct tone %s[] = {", notes_name)
		for _, t := range music {
			fmt.Printf("{%d,%d},", t.frequency, t.duration_ms)
		}
		fmt.Printf("};\n")
		fmt.Printf("const struct music melody = { %d, %s };\n", len(music), notes_name)
		return
	}

borked:
	log.Fatalf("Bad tune at location %d", idx)
}

func scan_int(tune []byte, idx int) (int, int) {
	lim := len(tune)
	num := 0
	for idx < lim && isdigit(tune[idx]) {
		num = (num * 10) + digval(tune[idx])
		idx++
	}
	return num, idx
}

func isdigit(c byte) bool {
	return c >= '0' && c <= '9'
}

func digval(c byte) int {
	return int(c - '0')
}
