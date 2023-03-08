// -*- fill-column: 100; indent-tabs-mode: t; tab-width: 2 -*-
//
// Simple web server that supports the SnappySense PoC in various ways.
//
// GET / returns the file index.html in the working directory
// GET /index.html ditto
//
// GET /snappy.js returns the snappy.js file in the working directory
//
// GET /devices returns a list of device objects as a JSON array, each object has
//   at least "device" (device ID) and "location" (device location ID) keys.
//   (Probably the device should also have last-seen, maybe other things.)
//
// GET /factors returns a list of factor objects as a JSON array, each object has
//   at least "factor" (factor ID) and "description" (factor description) keys
//
// GET /locations returns a list of location objects as a JSON array, each object has
//   at least "location" (location ID) and "description" (location description) keys
//   (Possibly the location should have a list of devices there.)
//
// GET /observations_by_device_and_factor?device=<device-id>&factor=<factor-id> returns
//   the observations for <factor-id> for the <device-id> as a JSON array, each object
//   in the array is a two-element array, [<sent-timestamp>, <observation-value>] where
//   the timestamp is an integer and the observation is a number (depends on the factor)
//
// GET /devices_by_location?location=<location-id> returns the device records for those
//   devices that are at the location, a JSON array
//
// GET /time returns a string that is the decimal number of seconds since
//   the Posix epoch, UTC.  (ie, it's a primitive time server.)
//
// POST /data takes a JSON object and writes it to a file.  It does not
//   check the validity of the object.

package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"strconv"
	"time"
)

// The port we're listening on
const PORT = 8086

// Obviously this data representation is dumb, but this is a simulation of a database, not a
// database.

const device_data = `
[{"device":"snp_1_1_no_1", "location":"ambulatory"},
 {"device":"snp_1_1_no_3", "location":"takterrassen"}]`

const location_data = `
[{"location":"ambulatory", "description":"Here and there"},
 {"location":"takterrassen", "description":"Roof terrace U1"}]`

var devices_by_location = map[string]string{
	"ambulatory":`[{"device":"snp_1_1_no_1", "location":"ambulatory"}]`,
	"takterrassen": `[{"device":"snp_1_1_no_3", "location":"takterrassen"}]`}

const factor_data = `
[{"factor":"temperature", "description": "temperature degrees C"},
 {"factor":"humidity", "description": "relative humidity %"},
 {"factor":"light", "description": "light lux"}]`

// TODO: Fill this in some more
var observations_by_device_and_factor = map[string]map[string]string{
	"snp_1_1_no_1": map[string]string{
		"temperature": `
[[1677665373,27.442245],[1677686346,24.096375],[1677678349,27.407532],[1677683839,27.185898],[1677669904,24.374084],[1677687058,25.677185],[1677674691,26.92421],[1677672836,25.848083],[1677684278,27.065735],[1677682344,27.338104],[1677687455,26.542358],[1677673975,28.326111],[1677677547,26.865463],[1677678771,27.818756],[1677673491,28.219299],[1677675826,26.504974],[1677674238,27.719955],[1677666150,25.714569],[1677676470,26.876144],[1677681905,25.890808],[1677684717,26.253967]]`,
		"humidity": `[]`,
		"light": `[]`},
	"snp_1_1_no_3": map[string]string{
		"temperature": `[[1677655373,21],[1677656346,22],[1677658349,23],[1677653839,22],[1677659904,21],[1677657058,22],[1677654691,22],[1677652836,22],[1677654278,23],[1677652344,24],[1677657455,25],[1677653975,25],[1677657547,26],[1677658771,27],[1677653491,26],[1677655826,25],[1677654238,24],[1677656150,23],[1677665373,23.442245],[1677686346,21.096375],[1677678349,23.407532],[1677683839,23.185898],[1677669904,21.374084],[1677687058,20.677185],[1677674691,21.92421],[1677672836,20.848083],[1677684278,23.065735],[1677682344,23.338104],[1677687455,21.542358],[1677673975,28.326111],[1677677547,21.865463],[1677678771,23.818756],[1677673491,28.219299],[1677675826,21.504974],[1677674238,23.719955],[1677666150,20.714569],[1677676470,21.876144],[1677681905,20.890808],[1677684717,21.253967]]`,
		"humidity": `[]`,
		"light": `[]`}}

func serve_static(request, content string) {
	http.HandleFunc(request, func(w http.ResponseWriter, r *http.Request) {
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(content))}
		w.Header()["Content-Type"] = []string{"application/json"}
		w.WriteHeader(200)
		w.Write([]byte(content))
	})
}

func serve_file(request, fn, ty string) {
	http.HandleFunc(request, func(w http.ResponseWriter, r *http.Request) {
		log.Print(request)
		if r.Method != "GET" {
			w.WriteHeader(403)
			fmt.Fprintf(w, "Bad method")
			return
		}
		f, err := os.Open(fn)
		if err != nil {
			w.WriteHeader(403)
			fmt.Fprintf(w, "No such file: %s", fn)
			return
		}
		b, err := io.ReadAll(f)
		if err != nil {
			w.WriteHeader(403)
			fmt.Fprintf(w, "I/O error")
			return
		}
		w.Header()["Content-Length"] = []string{strconv.Itoa(len(b))}
		w.Header()["Content-Type"] = []string{ty}
		w.WriteHeader(200)
		w.Write(b)
	})
}

func serve_time(request string) {
	http.HandleFunc(request, func(w http.ResponseWriter, r *http.Request) {
		log.Printf(request)
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
}

func receive_data(request, logfile string) {
	http.HandleFunc(request, func(w http.ResponseWriter, r *http.Request) {
		log.Printf(request)
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
			f, err := os.OpenFile(logfile, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
			if err == nil {
				f.Write(payload)
				f.Write([]byte{'\n'})
				f.Close()
			}
		}
	})
}

func serve_query(request string, process func (q url.Values) (string, bool)) {
	http.HandleFunc(request, func(w http.ResponseWriter, r *http.Request) {
		result, ok := process(r.URL.Query())
		if !ok {
			w.WriteHeader(400)
			fmt.Fprintf(w, "Bad request")
		} else {
			w.Header()["Content-Length"] = []string{strconv.Itoa(len(result))}
			w.Header()["Content-Type"] = []string{"application/json"}
			w.WriteHeader(200)
			w.Write([]byte(result))
		}
	})
}

func main() {
	serve_static("/devices", device_data)
	serve_static("/locations", location_data)
	serve_static("/factors", factor_data)

	serve_file("/", "index.html", "text/html")
	serve_file("/index.html", "index.html", "text/html")
	serve_file("/snappy.js", "snappy.js", "application/javascript")

	serve_time("/time")

	serve_query("/observations_by_device_and_factor",
		func (q url.Values) (string, bool) {
			devs, ok := q["device"]
			if !ok || len(devs) != 1 {
				return "", false
			}
			device := devs[0]
			facs, ok := q["factor"]
			if !ok || len(facs) != 1 {
				return "", false
			}
			factor := facs[0]
			if observations_by_device_and_factor[device] == nil ||
				observations_by_device_and_factor[device][factor] == "" {
				return "", false
			}
			return observations_by_device_and_factor[device][factor], ok
		})

	serve_query("/devices_by_location",
		func (q url.Values) (string, bool) {
			locs, ok := q["location"]
			if !ok || len(locs) == 0 {
				return "", false
			}
			loc := locs[0]
			if devices_by_location[loc] == "" {
				return "", false
			}
			return devices_by_location[loc], true
		})
	
	receive_data("/data", "snappy.log")

	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", PORT), nil))
}
