// -*- fill-column: 100; indent-tabs-mode: t; tab-width: 2 -*-
//
// Simple web server that supports the SnappySense PoC in various ways.
//
// GET / returns the file index.html in the working directory
//
// GET /devices returns a list of device objects as a JSON array
//
// GET /factors returns a list of factor objects as a JSON array
//
// GET /observations?device=<device-id>&factor=<factor-id> returns the observations for <factor-id>
// for the <device-id>
//
// GET /time returns a string that is the decimal number of seconds since
// the Posix epoch, UTC.  (ie, it's a primitive time server.)
//
// POST /data takes a JSON object and writes it to a file.  It does not
// check the validity of the object.

package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"
)

// The port we're listening on
const PORT = 8086

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/")
		if r.Method != "GET" {
			w.WriteHeader(403)
			fmt.Fprintf(w, "Bad method")
			return
		}
		f, err := os.Open("index.html")
		if err != nil {
			w.WriteHeader(403)
			fmt.Fprintf(w, "No such file")
			return
		}
		b, err := io.ReadAll(f)
		if err != nil {
			w.WriteHeader(403)
			fmt.Fprintf(w, "I/O error")
			return
		}
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(b))}
		w.Header()["Content-Type"] = []string{"text/html"}
		w.WriteHeader(200)
		w.Write(b)
	})

	http.HandleFunc("/devices", func(w http.ResponseWriter, r *http.Request) {
		result := "[{\"device\":\"snp_1_1_no_1\"},{\"device\":\"snp_1_1_no_2\"},{\"device\":\"snp_1_1_no_3\"}]"
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(result))}
		w.Header()["Content-Type"] = []string{"application/json"}
		w.WriteHeader(200)
		w.Write([]byte(result))
	})

	http.HandleFunc("/factors", func(w http.ResponseWriter, r *http.Request) {
		result := "[\"temperature\",\"humidity\",\"light\"]"
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(result))}
		w.Header()["Content-Type"] = []string{"application/json"}
		w.WriteHeader(200)
		w.Write([]byte(result))
	})

	http.HandleFunc("/observations", func(w http.ResponseWriter, r *http.Request) {
		// TODO: Different results for different devices and factors
		// TODO: Actually parse the header
		// This returns string values because the current lambda code does that, but
		// I think there's no reason why the lambda code couldn't convert these to numbers,
		// it would be more sensible for everyone.
		result := "[[\"1677665373\",\"27.442245\"],[\"1677686346\",\"24.096375\"],[\"1677678349\",\"27.407532\"],[\"1677683839\",\"27.185898\"],[\"1677669904\",\"24.374084\"],[\"1677687058\",\"25.677185\"],[\"1677674691\",\"26.92421\"],[\"1677672836\",\"25.848083\"],[\"1677684278\",\"27.065735\"],[\"1677682344\",\"27.338104\"],[\"1677687455\",\"26.542358\"],[\"1677673975\",\"28.326111\"],[\"1677677547\",\"26.865463\"],[\"1677678771\",\"27.818756\"],[\"1677673491\",\"28.219299\"],[\"1677675826\",\"26.504974\"],[\"1677674238\",\"27.719955\"],[\"1677666150\",\"25.714569\"],[\"1677676470\",\"26.876144\"],[\"1677681905\",\"25.890808\"],[\"1677684717\",\"26.253967\"]]"
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(result))}
		w.Header()["Content-Type"] = []string{"application/json"}
		w.WriteHeader(200)
		w.Write([]byte(result))
	})

	http.HandleFunc("/time", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/time")
		if r.Method != "GET" {
			w.WriteHeader(403)
			fmt.Fprintf(w, "Bad method")
			return
		}
		payload := fmt.Sprintf("%d", time.Now().Unix())
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(payload))}
		w.Header()["Content-Type"] = []string{"text/plain"}
		w.WriteHeader(200)
		w.Write([]byte(payload))
	})

	http.HandleFunc("/data", func(w http.ResponseWriter, r *http.Request) {
		log.Printf("/data")
		if r.Method != "POST" {
			w.WriteHeader(403)
			fmt.Fprintf(w, "Bad method")
			return
		}
		ct, ok := r.Header["Content-Type"]
		if !ok || ct[0] != "application/json" {
			w.WriteHeader(400)
			fmt.Fprintf(w, "Bad content-type")
			return
		}
		payload := make([]byte, r.ContentLength)
		_, err := r.Body.Read(payload)
		if err != nil && err != io.EOF {
			w.WriteHeader(400)
			fmt.Fprintf(w, "Bad content")
		} else {
			w.WriteHeader(202)
			f, err := os.OpenFile("snappy.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
			if err == nil {
				f.Write(payload)
				f.Write([]byte{'\n'})
				f.Close()
			}
		}
	})

	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", PORT), nil))
}
