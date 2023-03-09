// -*- fill-column: 100; tab-width: 2; indent-tabs-mode: t -*-
//
// Configuration language compiler.
//
// Translate a text configuration to its binary representation.  See README.md for a spec.
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

// Integer variables - these indices are fixed by the spec
const (
	ivar_enabled = 0
	ivar_obs_freq = 1
)

// String variables - these indices are fixed by the spec
const (
	svar_device_name = 0
	svar_ssid1 = 1
	svar_ssid2 = 2
	svar_ssid3 = 3
	svar_password1 = 4
	svar_password2 = 5
	svar_password3 = 6
)

// Instructions - these opcodes are fixed by the spec
const (
	instr_start byte = 1
	instr_stop byte = 2
	instr_seti byte = 3
	instr_sets byte = 4
)

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
	"enabled":               &varinfo{ty:int_ty, index:ivar_enabled},
	"observation-frequency": &varinfo{ty:int_ty, index:ivar_obs_freq},
	"device-name":           &varinfo{ty:str_ty, index:svar_device_name},
	"ssid1":                 &varinfo{ty:str_ty, index:svar_ssid1},
	"ssid2":                 &varinfo{ty:str_ty, index:svar_ssid2},
	"ssid3":                 &varinfo{ty:str_ty, index:svar_ssid3},
	"password1":             &varinfo{ty:str_ty, index:svar_password1},
	"password2":             &varinfo{ty:str_ty, index:svar_password2},
	"password3":             &varinfo{ty:str_ty, index:svar_password3},
}

type set_stmt struct {
	v *varinfo
	sval string										// If string
	ival int											// If int
}

type program struct {
	v0, v1, v2 byte								// version number
	clear bool										// flag: clear prefs before setting
	save bool											// flag: save prefs at end
	body []*set_stmt								// pref settings
}

func main() {
	if len(os.Args) != 3 {
		log.Fatalf("Wrong number of arguments.  Usage: %s infilename outfilename", os.Args[0])
	}
	infilename := os.Args[1]
	outfilename := os.Args[2]
	inp, err := read_input(infilename)
	if err != nil {
		log.Fatalf("Could not open " + infilename)
	}
	prog := parse(inp)
	outp := new_output()
	codegen(prog, outp)
	if outp.dump(outfilename) != nil {
		log.Fatalf("Could not write " + outfilename)
	}
}

// Parsing and syntax checking and IR generation, including checking variable names and types.
// TODO maybe return errors here instead of calling log.Fatalf

func parse(inp *input) *program {
	prog := &program{ v0:0, v1:0, v2:0, clear:false, save:false, body:[]*set_stmt{} }

	// Look for `version`, it is mandatory
	l, lineno, ok := inp.next()
	if !ok {
		log.Fatalf("Empty program")
	}
	if x, y, z, ok := match_version(l); ok {
		// We know about: 1.0.0
		if x == 1 && y == 0 && z == 0 {
			prog.v0, prog.v1, prog.v2 = x, y, z
		} else {
			log.Fatalf("Line %d: Unknown version", lineno)
		}
	} else {
		log.Fatalf("Line %d: First statement must be `version`", lineno)
	}

	// Look for `clear`, it is optional
	l, lineno, ok = inp.next()
	if ok && match_clear(l) {
		prog.clear = true
		l, lineno, ok = inp.next()
	}

	// Loop over statements, exit on `end` or `save` or end of input
	for ok {
		if match_save(l) || match_end(l) {
			break
		}
		if name, val, matched := match_set(l); matched {
			the_var, found := variables[name]
			if !found {
				log.Fatalf("Line %d: Unknown variable %s", lineno, name)
			}
			switch (the_var.ty) {
			case int_ty:
				ival, err := strconv.Atoi(val)
				if err != nil {
					log.Fatalf("Line %d: Bad integer value %s", lineno, val)
				}
				prog.body = append(prog.body, &set_stmt{v:the_var, ival: ival})
			case str_ty:
				if len(val) > 65535 {
					log.Fatalf("Line %d: String too long", lineno)
				}
				prog.body = append(prog.body, &set_stmt{v:the_var, sval: val})
			default:
				panic("Should not happen")
			}
			l, lineno, ok = inp.next()
			continue
		}
		if _, _, matched := match_cert(l); matched {
			// TODO
			// Lookup variable name, gets us a type and index
			// Make sure val matches the name
			// Maybe read a file if the val is a file indirect, better to do it here
			// than in codegen
			// Make sure the value read is not too long
			l, lineno, ok = inp.next()
			continue
		}
		if _, _, _, matched := match_version(l); matched {
			log.Fatalf("Line %d: `version` not allowed here", lineno)
		}
		if match_clear(l) {
			log.Fatalf("Line %d: `clear` not allowed here", lineno)
		}
		log.Fatalf("Line %d: Unknown statement %s", lineno, l)
	}

	// Look for `end` or `save` and handle premature end
	if !ok {
		log.Fatalf("Line %d: Requires `end` or `save`", lineno)
	} else if match_save(l) {
		prog.save = true
	} else if match_end(l) {
		prog.save = false
	}

	// Make sure we're done
	if _, _, ok = inp.next(); ok {
		log.Fatalf("Line %d: Input follows `end` or `save`", lineno)
	}

	return prog
}

func match_clear(l string) bool {
	return l == "clear"
}

func match_end(l string) bool {
	return l == "end"
}

func match_save(l string) bool {
	return l == "save"
}

var version_re = regexp.MustCompile(`^version\s+(\d+)\.(\d+)\.(\d+)$`)

func match_version(l string) (v0, v1, v2 byte, ok bool) {
	matches := version_re.FindSubmatch([]byte(l))
	if matches == nil {
		return
	}
	v0i, e0 := strconv.Atoi(string(matches[1]))
	v1i, e1 := strconv.Atoi(string(matches[2]))
	v2i, e2 := strconv.Atoi(string(matches[3]))
	if e0 != nil || e1 != nil || e2 != nil || v0i < 0 || v0i > 255 || v1i < 0 || v1i > 255 || v2i < 0 || v2i > 255 {
		return
	}
	v0 = byte(v0i)
	v1 = byte(v1i)
	v2 = byte(v2i)
	ok = true
	return
}

var set_re = regexp.MustCompile(`^set\s+([a-z0-9_-]+)\s+(.*)$`)

func match_set(l string) (name string, val string, ok bool) {
	matches := set_re.FindSubmatch([]byte(l))
	if matches == nil {
		return
	}
	name = string(matches[1])
	v := string(matches[2])
	if v[0] == '"' {
		if len(v) < 2 || v[len(v)-1] != '"' {
			return
		} 
		val = v[1:len(v)-1]
		// TODO: Convert \" in the string to plain ", maybe also other escape sequences
	} else {
		// Literal value, no escape sequences are processed
		val = v
	}
	ok = true
	return
}

func match_cert(l string) (name string, val string, ok bool) {
	// TODO
	// As match_set, mostly, but require indirection through @filename.
	// Allow @filename to be quoted: "@filename", strip quotes as for set
	// Expand @~/... to @$HOME/..., fail if $HOME does not exist
	// The filename is not interpreted here past that, and it's possible tilde
	// expansion should not be done here either
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
		switch (s.v.ty) {
		case int_ty:
			out.emitn(instr_seti, byte(s.v.index), byte(s.ival), byte(s.ival >> 8), byte(s.ival >> 16), byte(s.ival >> 24))

		case str_ty:
			// Parser has checked that length is within bounds, we just blast out the utf8 representation
			out.emitn(instr_sets, byte(s.v.index), byte(len(s.sval)), byte(len(s.sval) >> 8))
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
	lineno    int									// private
	lines     []string						// private
}

func read_input(infilename string) (*input, error) {
	lines := []string{}
	infile, err := os.Open(infilename)
	if err != nil {
		return nil, err
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
			return nil, err
		}
		lines = append(lines, l)
	}
	return &input{ lineno: 0, lines: lines }, nil
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
	bytes     []byte							// private
}

func new_output() *output {
	return &output{ bytes: []byte{} }
}

func (self *output) emitn(bytes ...byte) {
	self.bytes = append(self.bytes, bytes...)
}

func (self *output) emit(bytes []byte) {
	self.bytes = append(self.bytes, bytes...)
}

func (self *output) dump(outfilename string) error {
	outf, err := os.Create(outfilename)
	if err != nil {
		return err
	}
	defer outf.Close()
	_, err = outf.Write(self.bytes)
	return err
}

