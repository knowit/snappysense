This is a dead-simple http server that can be used as a time server for the
snappysense client and to receive data through the client's "web upload"
feature.

To compile, `go build`.

To run, `go run httpsrv`.

The server listens on port 8086, see httpsrv.go.

The server stores received JSON data as individual lines in snappy.log.  It does nothing
to check the validity of those data.

To process the log into a csv file, you can perhaps use the json2csv script.
