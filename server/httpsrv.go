// Simple web server that supports the SnappySense PoC.
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
  "os"
  "net/http"
  "strconv"
  "time"
)

// The port we're listening on
const PORT = 8086;

func main() {
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
    payload := make([]byte, r.ContentLength);
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


