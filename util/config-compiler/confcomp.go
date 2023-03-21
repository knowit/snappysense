// -*- fill-column: 100; tab-width: 2; indent-tabs-mode: t -*-
//
// Configuration language compiler.
//
// Translate a text configuration to its binary representation.  See README.md for a spec and some
// motivation.
//
// Usage:
//   confcomp input-file output-file

package main

import (
	"bufio"
	"io"
	"log"
	"os"
	"regexp"
	"strconv"
	"strings"
)

// Instructions - these opcodes are fixed by the spec.  All operands below are single bytes, see
// spec for more.
const (
	instr_start byte = 1 // START v0 v1 v2 flag
	instr_stop  byte = 2 // STOP flag
	instr_seti  byte = 3 // SETI v ii ii ii ii
	instr_sets  byte = 4 // SETS v uu uu byte ...
)

// Integer variables - these indices are fixed by the spec
const (
	ivar_enabled              = 0
	ivar_observation_interval = 1
	ivar_upload_interval      = 2
	ivar_mqtt_endpoint_port   = 3
	ivar_mqtt_use_tls         = 4
)

// String variables - these indices are fixed by the spec
const (
	svar_device_id          = 0
	svar_device_class       = 1
	svar_ssid1              = 2
	svar_ssid2              = 3
	svar_ssid3              = 4
	svar_password1          = 5
	svar_password2          = 6
	svar_password3          = 7
	svar_mqtt_endpoint_host = 8
	svar_mqtt_id            = 9
	svar_mqtt_root_cert     = 10
	svar_mqtt_auth          = 11
	svar_mqtt_device_cert   = 12
	svar_mqtt_private_key   = 13
	svar_mqtt_username      = 14
	svar_mqtt_password      = 15
)

// Limits - these values are fixed by the spec, more or less
//
// Validation guards against stupidity / errors, but not against attacks since the binary form is
// unsigned and can be produced by any program.
const (
	max_version_value        = 255
	max_string_length        = 65535
	min_observation_interval = 60
	max_observation_interval = 8 * 60 * 60
	max_upload_interval      = 48 * 60 * 60
	min_inet_port            = 1000  // Investigate
	max_inet_port            = 65535 // Investigate
)

type int_limit struct {
	min, max int
}

var int_limits = map[int]int_limit{
	ivar_enabled:              int_limit{min: 0, max: 1},
	ivar_observation_interval: int_limit{min: min_observation_interval, max: max_observation_interval},
	ivar_upload_interval:      int_limit{min: min_observation_interval, max: max_upload_interval},
	ivar_mqtt_endpoint_port:   int_limit{min: min_inet_port, max: max_inet_port},
}

// Mapping from names to variable information: type and index
type var_ty int

const (
	int_ty var_ty = iota
	str_ty
)

type varinfo struct {
	ty    var_ty
	index int
}

var variables = map[string]*varinfo{
	// Integers
	"enabled":              &varinfo{ty: int_ty, index: ivar_enabled},
	"mqtt-endpoint-port":   &varinfo{ty: int_ty, index: ivar_mqtt_endpoint_port},
	"observation-interval": &varinfo{ty: int_ty, index: ivar_observation_interval},
	"upload-interval":      &varinfo{ty: int_ty, index: ivar_upload_interval},
	"mqtt-use-tls":         &varinfo{ty: int_ty, index: ivar_mqtt_use_tls},

	// Strings
	"device-id":          &varinfo{ty: str_ty, index: svar_device_id}, // TODO: Validate form
	"device-class":       &varinfo{ty: str_ty, index: svar_device_class},         // TODO: Validate form
	"ssid1":              &varinfo{ty: str_ty, index: svar_ssid1},
	"ssid2":              &varinfo{ty: str_ty, index: svar_ssid2},
	"ssid3":              &varinfo{ty: str_ty, index: svar_ssid3},
	"password1":          &varinfo{ty: str_ty, index: svar_password1},
	"password2":          &varinfo{ty: str_ty, index: svar_password2},
	"password3":          &varinfo{ty: str_ty, index: svar_password3},
	"mqtt-endpoint-host": &varinfo{ty: str_ty, index: svar_mqtt_endpoint_host}, // TODO: Validate form
	"mqtt-id":            &varinfo{ty: str_ty, index: svar_mqtt_id},            // TODO: Validate form
	"mqtt-root-cert":     &varinfo{ty: str_ty, index: svar_mqtt_root_cert},     // TODO: Validate form
	"mqtt-device-cert":   &varinfo{ty: str_ty, index: svar_mqtt_device_cert},   // TODO: Validate form
	"mqtt-private-key":   &varinfo{ty: str_ty, index: svar_mqtt_private_key},   // TODO: Validate form
	"mqtt-username":      &varinfo{ty: str_ty, index: svar_mqtt_username},
	"mqtt-password":      &varinfo{ty: str_ty, index: svar_mqtt_password},
	"mqtt-auth":          &varinfo{ty: str_ty, index: svar_mqtt_auth}, // TODO: Validate values, "pass" or "x509"
}

type set_stmt struct {
	v    *varinfo
	sval string // If string
	ival int    // If int
}

type program struct {
	v0, v1, v2 byte        // version number
	clear      bool        // flag: clear prefs before setting
	save       bool        // flag: save prefs at end
	body       []*set_stmt // pref settings
}

func main() {
	if len(os.Args) != 3 {
		log.Fatalf("Wrong number of arguments.  Usage: %s infilename outfilename", os.Args[0])
	}
	infilename := os.Args[1]
	outfilename := os.Args[2]
	inp := read_input(infilename)
	prog := parse(inp)
	outp := new_output()
	codegen(prog, outp)
	outp.dump(outfilename)
}

// Parsing and syntax checking and IR generation, including checking variable names and types.

func parse(inp *input) *program {
	prog := &program{v0: 0, v1: 0, v2: 0, clear: false, save: false, body: []*set_stmt{}}

	// Seed the input queue
	l, lineno, have_line := inp.next()

	// Look for `snappysense-compiled-config`, it is mandatory
	if !have_line {
		log.Fatalf("Empty program")
	}
	if x, y, z, matched := match_preamble(l, lineno); matched {
		// We know about: 1.0.0
		if x == 1 && y == 0 && z == 0 {
			prog.v0, prog.v1, prog.v2 = x, y, z
		} else {
			log.Fatalf("Line %d: Unknown version", lineno)
		}
	} else {
		log.Fatalf("Line %d: First statement must be `snappysense-compiled-config`", lineno)
	}
	l, lineno, have_line = inp.next()

	// Look for `clear`, it is optional
	if have_line && l == "clear" {
		prog.clear = true
		l, lineno, have_line = inp.next()
	}

	for have_line && l != "save" && l != "end" {

		if name, val, matched := match_set(l, lineno); matched {
			the_var, found := variables[name]
			if !found {
				log.Fatalf("Line %d: Unknown variable %s", lineno, name)
			}

			switch the_var.ty {
			case int_ty:
				ival, err := strconv.Atoi(val)
				if err != nil {
					log.Fatalf("Line %d: Bad integer value %s", lineno, val)
				}
				if limits, found := int_limits[the_var.index]; found {
					if ival < limits.min || ival > limits.max {
						log.Fatalf("Line %d: Out of range for %s: %d", lineno, name, ival)
					}
				}
				prog.body = append(prog.body, &set_stmt{v: the_var, ival: ival})

			case str_ty:
				if len(val) > max_string_length {
					log.Fatalf("Line %d: String too long", lineno)
				}
				prog.body = append(prog.body, &set_stmt{v: the_var, sval: val})

			default:
				panic("Should not happen")
			}

			l, lineno, have_line = inp.next()
			continue
		}

		log.Fatalf("Line %d: Illegal statement in this context: %s", lineno, l)
	}

	// Look for `end` or `save` and handle premature end
	if !have_line {
		log.Fatalf("Line %d: Requires `end` or `save`", lineno)
	} else if l == "save" {
		prog.save = true
		l, lineno, have_line = inp.next()
	} else if l == "end" {
		prog.save = false
		l, lineno, have_line = inp.next()
	} else {
		panic("Should not happen")
	}

	// Make sure we're done
	if have_line {
		log.Fatalf("Line %d: Input follows `end` or `save`: %s", lineno, l)
	}

	return prog
}

var preamble_re = regexp.MustCompile(`^snappysense-compiled-config\s+(\d+)\.(\d+)\.(\d+)$`)

func match_preamble(l string, lineno int) (v0, v1, v2 byte, matched bool) {
	matches := preamble_re.FindSubmatch([]byte(l))
	if matches == nil {
		if strings.HasPrefix(l, "snappysense-compiled-config") {
			log.Fatalf("Line %d: Bad syntax for `snappysense-compiled-config`: %s", lineno, l)
		}
		return
	}
	v0i, e0 := strconv.Atoi(string(matches[1]))
	v1i, e1 := strconv.Atoi(string(matches[2]))
	v2i, e2 := strconv.Atoi(string(matches[3]))
	if e0 != nil || e1 != nil || e2 != nil ||
		v0i < 0 || v0i > max_version_value ||
		v1i < 0 || v1i > max_version_value ||
		v2i < 0 || v2i > max_version_value {
		log.Fatalf("Line %d: bad version value: %s", lineno, l)
	}
	v0 = byte(v0i)
	v1 = byte(v1i)
	v2 = byte(v2i)
	matched = true
	return
}

var set_re = regexp.MustCompile(`^set\s+([a-zA-Z][a-zA-Z0-9_-]*)\s+(.*)$`)

func match_set(l string, lineno int) (name string, val string, matched bool) {
	matches := set_re.FindSubmatch([]byte(l))
	if matches == nil {
		if strings.HasPrefix(l, "set") {
			log.Fatalf("Line %d: Bad syntax for set/setf: %s", lineno, l)
		}
		return
	}
	name = string(matches[1])
	v := string(matches[2])
	indirect := false
	if v[0] == '@' {
		indirect = true
		v = v[1:]
	}
	if len(v) > 0 && v[0] == '"' {
		if len(v) < 2 || v[len(v)-1] != '"' {
			log.Fatalf("Line %d: Bad value: %s", v)
		}
		val = v[1 : len(v)-1]
		// TODO: Convert \" in the string to plain ", maybe also other escape sequences
	} else {
		// Literal value, no escape sequences are processed
		val = v
	}
	if indirect {
		// TODO: If the file name is relative, should it not be relative to the input file, not the cwd?
		if strings.HasPrefix(val, "~/") {
			if home, found := os.LookupEnv("HOME"); found {
				// TODO: Deal with HOME not being an absolute path
				// TODO: Deal with unsafe paths
				val = home + val[1:]
			} else {
				log.Fatalf("Line %d: $HOME not defined for tilde expansion", lineno)
				return
			}
		}
		if len(val) == 0 {
			log.Fatalf("Line %d: Empty filename")
		}
		// TODO: Deal with paths containing eg ".."
		filename := val
		inp, err := os.Open(filename)
		if err != nil {
			log.Fatalf("Line %d: Could not open %s: %w", lineno, filename, err)
		}
		defer inp.Close()
		blob, err := io.ReadAll(inp)
		if err != nil {
			log.Fatalf("Line %d: Could not read %s: %w", lineno, filename, err)
		}
		val = string(blob)
	}
	matched = true
	return
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Codegen is very simple, just blasting out bits into a stream

func codegen(prog *program, out *output) {
	clearb := byte(0)
	if prog.clear {
		clearb = byte(1)
	}
	out.emitn(instr_start, prog.v0, prog.v1, prog.v2, clearb)
	for _, s := range prog.body {
		switch s.v.ty {
		case int_ty:
			out.emitn(instr_seti, byte(s.v.index), byte(s.ival), byte(s.ival>>8), byte(s.ival>>16), byte(s.ival>>24))

		case str_ty:
			// Parser has checked that length is within bounds, we just blast out the utf8 representation
			out.emitn(instr_sets, byte(s.v.index), byte(len(s.sval)), byte(len(s.sval)>>8))
			out.emit([]byte(s.sval))

		default:
			panic("Should not happen")
		}
	}
	saveb := byte(0)
	if prog.save {
		saveb = byte(1)
	}
	out.emitn(instr_stop, saveb)
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Input stream of non-blank non-comment lines

type input struct {
	lineno int      // private
	lines  []string // private
}

func read_input(infilename string) *input {
	lines := []string{}
	infile, err := os.Open(infilename)
	if err != nil {
		log.Fatalf("Could not open %s: %w", infilename, err)
	}
	defer infile.Close()
	reader := bufio.NewReader(infile)
	for {
		// This should be OK for \r\n files as the \r should be stripped as whitespace
		l, err := reader.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				lines = append(lines, l)
				break
			}
			log.Fatalf("Could not read %s: %w", infilename, err)
		}
		lines = append(lines, l)
	}
	return &input{lineno: 0, lines: lines}
}

// Get the next non-blank non-comment line, stripped of leading and trailing spaces

func (self *input) next() (string, int, bool) {
	for {
		if self.lineno == len(self.lines) {
			return "", 0, false
		}

		line := strings.TrimSpace(self.lines[self.lineno])
		self.lineno++

		if line == "" || line[0] == '#' {
			continue
		}

		return line, self.lineno, true
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Output stream of bytes

type output struct {
	bytes []byte // private
}

func new_output() *output {
	return &output{bytes: []byte{}}
}

func (self *output) emitn(bytes ...byte) {
	self.bytes = append(self.bytes, bytes...)
}

func (self *output) emit(bytes []byte) {
	self.bytes = append(self.bytes, bytes...)
}

func (self *output) dump(outfilename string) {
	outf, err := os.Create(outfilename)
	if err != nil {
		log.Fatalf("Could not create %s: %w", outfilename, err)
	}
	defer outf.Close()
	_, err = outf.Write(self.bytes)
	if err != nil {
		log.Fatalf("Could not write output to %s: %w", outfilename, err)
	}
}
