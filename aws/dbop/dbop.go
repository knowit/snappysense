// -*- indent-tabs-mode: t; tab-width: 2; fill-column: 100 -*-
//
// Simple command-line interface to the SnappySense DynamoDB databases.
//
// This also serves as the (executable) definition of the SnappySense data model.
//
// See `helptext` below for all information about how to run this program.
//
// NOTE!  This gets the aws config and credentials through default mechanisms in the SDK: it reads
// environment variables if they are set and otherwise gets the config from ~/.aws/config and the
// aws identity from ~/.aws/credentials.

// Useful links:
//
// https://pkg.go.dev/github.com/aws/aws-sdk-go-v2/service/dynamodb
//
// https://github.com/aws/aws-sdk-go-v2/tree/main/service/dynamodb
// https://github.com/aws/aws-sdk-go-v2/blob/main/service/dynamodb/types/types.go
// https://github.com/aws/aws-sdk-go-v2/blob/main/service/dynamodb/types/enums.go

package main

import (
	"context"
	"encoding/gob"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/dynamodb"
	"github.com/aws/aws-sdk-go-v2/service/dynamodb/types"
)

const helptext string = `
Usage:
 dbop help
 dbop <table> <verb> <argument> ...

The "table" is one of "device", "location", "class", "factor", "observation" or 
one of those names prefixed with "dev_", to use a separate "development" table
for experimentation.

The "verb" and "argument" combinations are as follows.

  create-table
    ; Create the table in the database

  delete-table
    ; Delete the table from the database

  list
    ; Lists all records in the table, sorted lexicographically by primary key,
    ; with fields chosen by this program.

  dump filename
    ; exports all records in the table to a binary file (see undump).

  undump filename
    ; Imports records to the from the given file.  The file must have been created
    ; by "dump".  There is currently no checking that the dump fits the table, or
    ; that the table format has not changed since the dump was made.

  info
    ; Show information about the records in the table, notably the fields required
    ; to create a new record and the name of the keys and so on.

  add fieldname=value fieldname=value ...
    ; Add a new record.  All required fields must be present.  Replaces any record
    ; with the same primary key; does not augment it with new data.  Will add
    ; default values for non-required fields that are not present on command line.
    ; If you need to quote something, do it like this: "fieldname=value with spaces"

  get value
    ; Looks up the record with the given value for the primary key.

  delete value
    ; Deletes the record with the given value for the primary key.

  relocate-if device-id old-location-id new-location-id
    ; The "table" must be "observation" or "dev_observation".  Observations for the
    ; given device that know their location as the "old-location-id" are moved to
    ; the "new-location-id".  This is a pretty crude clean-up mechanism for devices
    ; that were misregistered.

For example

  dbop location add class=toppen "description=Takterrassen i U1"
  dbop device list
  dbop device get snp_1_1_no_2
  dbop observation relocate-if snp_1_1_no_3 old=ambulatory new=u1_5_hotdesk
`

// Names in Field are short to make some of the tables below legible
type Field struct {
	name  string // `device`, etc
	ty    string // one of the TY_ values defined below
	hide  bool   // true if omitted in a listing
	opt   bool   // True if optional
	def   string // Optional default value if optional; even for numbers this is a string
	gloss string // What the field is for
}

type Table struct {
	short_name string // `table`, etc
	real_name  string // `snappy_table`, etc
	key_name   string // primary key field name
	fields     []*Field
	gloss      string	// What the table is for
}

// For timestamps the epoch is *always* the Unix epoch, 1/1/1970 00:00:00 UTC.  This means in
// particular that devices on LoRaWAN, where the epoch is the GPS epoch 1/1/1980 00:00:00 UTC, will
// have to make a local adjustment before transmitting.
//
// Device timestamps before 1676531699 (16 February 2023 07:15:xx UTC) are definitely from
// mis-configured or un-configured devices.

const (
	TY_S  = "string"							// "S"
	TY_I  = "int"									// "N"
	TY_B  = "bool"								// "BOOL"
	TY_N  = "number"							// "N"
	TY_SL = "string list"					// "L" holding "S" nodes, because AWS disallows empty "SS"
	TY_ST = "server_timestamp"		// "N", UTC seconds since Unix epoch server-side
	TY_DT = "device_timestamp"		// "N", UTC seconds since Unix epoch client-side
)

// The command line parser allows `dev_table` in addition to `table`, if if so,
// it updates these tables to `dev_table` for the short name and `dev_snappy_table`
// for the real name.

var tables = []*Table{
	&Table{
		short_name: "device", real_name: "snappy_device", key_name: "device",
		fields: []*Field{

			&Field{name: "device", ty: TY_S,
				gloss: "Unique device ID, defined by this table"},

			&Field{name: "class", ty: TY_S,
				gloss: "Class ID, shall exist in CLASS"},

			&Field{name: "last_contact", ty: TY_ST, opt: true, def: "0",
				gloss: "Timestamp (server time) of last message from the device, UTC seconds since Unix epoch"},

			&Field{name: "location", ty: TY_S, opt: true,
				gloss: "Location ID, shall exist in LOCATION"},

			&Field{name: "enabled", ty: TY_B, opt: true, def: "true",
				gloss: "Whether device should report observation data or not"},

			&Field{name: "interval", ty: TY_I, opt: true, def: "3600",
				gloss: "Interval between observations recorded for upload, in seconds"},

			&Field{name: "factors", ty: TY_SL,
				gloss: "Factors observable by device, all shall exist in FACTOR"},
		},
		gloss: "Defines the devices, holds information about each device"},

	&Table{
		short_name: "class", real_name: "snappy_class", key_name: "class",
		fields: []*Field{

			&Field{name: "class", ty: TY_S,
				gloss: "Unique class ID, defined by this table"},

			&Field{name: "description", ty: TY_S,
				gloss: "Arbitrary description of the class"},
		},
		gloss: "Defines the device class"},

	&Table{
		short_name: "factor", real_name: "snappy_factor", key_name: "factor",
		fields: []*Field{

			&Field{name: "factor", ty: TY_S,
				gloss: "Unique factor name, defined by this table"},

			&Field{name: "description", ty: TY_S,
				gloss: "Arbitrary description of the factor"},
		},
		gloss: "Defines the environmental factors that can be observed by the devices"},

	&Table{
		short_name: "location", real_name: "snappy_location", key_name: "location",
		fields: []*Field{

			&Field{name: "location", ty: TY_S,
				gloss: "Unique location ID, defined by this table"},

			&Field{name: "description", ty: TY_S,
				gloss: "Arbitrary description of the location"},

			&Field{name: "devices", ty: TY_SL,
				gloss: "Devices at this location, all shall exist in DEVICE"},

			&Field{name: "timezone", ty: TY_S, opt: true, def: "",
				gloss: "A time zone area name (eg Europe/Berlin), as from iana.org/time-zones or timezonedb.com"},
		},
		gloss: "Defines the locations where devices can be" },

	&Table{
		short_name: "observation", real_name: "snappy_observation", key_name: "key",
		fields: []*Field{

			&Field{name: "key", ty: TY_S,
				gloss: "Synthesized unique key, combining device ID, arrival time, and other things"},

			&Field{name: "device", ty: TY_S,
				gloss: "The device that reported this observation, it shall exist in DEVICE"},

			&Field{name: "location", ty: TY_S, opt: true, def: "",
				gloss: "The presumed location of the device, if present it shall exist in LOCATION"},

			&Field{name: "sequenceno", ty: TY_I,
				gloss: "The sequence number reported by the device for this observation"},

			&Field{name: "received", ty: TY_ST,
				gloss: "Timestamp (server time) when the observation was received, UTC seconds since Unix epoch"},

			&Field{name: "sent", ty: TY_DT,
				gloss: "Timestamp (device time) when the observation was made, UTC seconds since Unix epoch"},
		},
		gloss: `Log of incoming observations.  Note, there are additional fields here, one per factor
  observed by the device, each factor name prefixed by "F#", eg, "F#temperature".`},
}

// For table creation.  The value "5" is from the AWS DynamoDB dashboard, good enough?
const (
	PROVISIONED_READ_CAPACITY_UNITS  = 5
	PROVISIONED_WRITE_CAPACITY_UNITS = 5
)

const (
	NONE = iota
	ADD
	CREATE_TABLE
	DELETE
	DELETE_TABLE
	INFO
	SCAN
	GET
  RELOCATE_IF
	DUMP
	UNDUMP
)

func main() {
	if len(os.Args) >= 2 && os.Args[1] == "help" {
		help()
		return
	}

	// the_table is the requested table
	// op is the operation
	// params is key-value pairs for `add`
	// key_value is the primary key value for `delete`, `get`

	the_table, op, params, key_value := parse_command_line()

	if op == INFO {
		show_table_info(the_table)
		return
	}

	cfg, err := config.LoadDefaultConfig(context.TODO())
	if err != nil {
		log.Fatalf("unable to load SDK config\n%v", err)
	}
	svc := dynamodb.NewFromConfig(cfg)

	switch op {
	case CREATE_TABLE:
		create_table(svc, the_table)

	case DELETE_TABLE:
		delete_table(svc, the_table)

	case SCAN:
		scan_table(svc, the_table)

	case DUMP:
		dump_table(svc, the_table, key_value)

	case GET:
		get_table_element(svc, the_table, key_value)

	case ADD:
		add_table_element(svc, the_table, params)

	case UNDUMP:
		undump_table(svc, the_table, key_value)

	case DELETE:
		delete_table_element(svc, the_table, key_value)

	case RELOCATE_IF:
		relocate_devices(svc, the_table, key_value, params)

	default:
		panic("Should not happen")
	}
}

func show_table_info(the_table *Table) {
	fmt.Printf("Table %s\n", the_table.short_name)
  if the_table.gloss != "" {
		fmt.Printf("  %s\n", the_table.gloss)
	}
	for _, f := range the_table.fields {
		req := "Required field"
		if f.opt {
			req = "Optional field"
		}
		fmt.Printf("  %s '%s', %s\n", req, f.name, f.ty)
    if f.name == the_table.key_name {
			fmt.Printf("    Primary key\n");
		}
		if f.opt && f.def != "" {
			fmt.Printf("    Default '%s'\n", f.def)
		}
		if f.gloss != "" {
			fmt.Printf("    %s\n", f.gloss);
		}
	}
}

func create_table(svc *dynamodb.Client, the_table *Table) {
	key_field := get_key_field(the_table)
	if key_field.ty != TY_S {
		log.Fatalf("Arbitrary restriction: Only string key fields are supported")
	}
	var rcu int64 = PROVISIONED_READ_CAPACITY_UNITS
	var wcu int64 = PROVISIONED_WRITE_CAPACITY_UNITS
	_, err := svc.CreateTable(context.TODO(), &dynamodb.CreateTableInput{
		AttributeDefinitions: []types.AttributeDefinition{
			types.AttributeDefinition{
				AttributeName: &the_table.key_name,
				AttributeType: types.ScalarAttributeTypeS,
			},
		},
		KeySchema: []types.KeySchemaElement{
			types.KeySchemaElement{
				AttributeName: &the_table.key_name,
				KeyType:       types.KeyTypeHash,
			},
		},
		TableName: &the_table.real_name,
		ProvisionedThroughput: &types.ProvisionedThroughput{
			ReadCapacityUnits:  &rcu,
			WriteCapacityUnits: &wcu,
		},
	})
	if err != nil {
		log.Fatalf("failed to create table %s\n%v", the_table.short_name, err)
	}
	fmt.Printf("Table %s added\n", the_table.short_name)
}

func delete_table(svc *dynamodb.Client, the_table *Table) {
	_, err := svc.DeleteTable(context.TODO(), &dynamodb.DeleteTableInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to delete table %s\n%v", the_table.short_name, err)
	}
	fmt.Printf("Table %s deleted\n", the_table.short_name)
}

func scan_table(svc *dynamodb.Client, the_table *Table) {
	// Build the request with its input parameters
	resp, err := svc.Scan(context.TODO(), &dynamodb.ScanInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to scan table %s\n%v", the_table.short_name, err)
	}
	for _, row := range resp.Items {
		display_row(the_table, row)
	}
}

func format_value(v types.AttributeValue) string {
	if w, ok := v.(*types.AttributeValueMemberS); ok {
		return w.Value
	}
	if w, ok := v.(*types.AttributeValueMemberN); ok {
		return w.Value
	}
	if w, ok := v.(*types.AttributeValueMemberBOOL); ok {
		if w.Value {
			return "true"
		}
		return "false"
	}
	if w, ok := v.(*types.AttributeValueMemberL); ok {
		s := ""
		for _, x := range w.Value {
			if s != "" {
				s += ", "
			}
			s += format_value(x)
		}
		return "[" + s + "]"
	}
	return fmt.Sprint(v)
}

func display_row(the_table *Table, row map[string]types.AttributeValue) {
	handled := make(map[string]bool, 10)
	handled[the_table.key_name] = true
	fmt.Printf("%s key=%s\n", the_table.short_name, the_table.key_name)
	for _, f := range the_table.fields {
		if !f.hide {
			if v, ok := row[f.name]; ok {
				fmt.Printf("  %s: %v\n", f.name, format_value(v))
				handled[f.name] = true
			}
		}
	}
	for k, v := range row {
		if handled[k] {
			continue;
		}
		fmt.Printf("  %s: %v\n", k, format_value(v))
		handled[k] = true
	}
}

func has_value(m map[string]string, v string) bool {
	_, found := m[v]
	return found
}

func add_table_element(svc *dynamodb.Client, the_table *Table, params map[string]string) {
	row := make(map[string]types.AttributeValue, 10)
	for _, f := range the_table.fields {
		if required_but_not_expressible(f) {
			add_defdef_attribute(row, f)
		} else if !f.opt || has_value(params, f.name) {
			add_attribute(row, f, params[f.name])
		} else {
			if f.def != "" {
				add_attribute(row, f, f.def)
			} else {
				add_defdef_attribute(row, f)
			}
		}
	}
	_, err := svc.PutItem(context.TODO(), &dynamodb.PutItemInput{
		TableName: &the_table.real_name,
		Item:      row,
	})
	if err != nil {
		log.Fatalf("failed to add to table %s\n%v", the_table.short_name, err)
	}
	log.Println("Item added")
}

func get_table_element(svc *dynamodb.Client, the_table *Table, key_value string) {
	items := make(map[string]types.AttributeValue, 1)
	key_field := get_key_field(the_table)
	add_attribute(items, key_field, key_value)
	resp, err := svc.GetItem(context.TODO(), &dynamodb.GetItemInput{
		TableName: &the_table.real_name,
		Key:       items,
	})
	if err != nil {
		log.Fatalf("failed to add to table %s\n%v", the_table.short_name, err)
	}
	display_row(the_table, resp.Item)
}

// A dump contains a `gob` encoding of a slice of rows (the Items field of a scan result).
//
// There is no versioning or checking on ingest that the dumped data make sense for the
// table they're being inserted into.  As long as there is an entry for the keys required
// by the table, the undump operation will likely succeed.

func dump_table(svc *dynamodb.Client, the_table *Table, filename string) {
	// Build the request with its input parameters
	resp, err := svc.Scan(context.TODO(), &dynamodb.ScanInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to scan table %s\n%v", the_table.short_name, err)
	}
	f, err := os.Create(filename)
	if err != nil {
		log.Fatal("failed to create file %s\n%v", filename, err)
	}
	defer f.Close()
	register_gob_types()
	enc := gob.NewEncoder(f)
	err = enc.Encode(resp.Items)
	if err != nil {
		log.Fatal("could not encode: %v", err)
	}
}

func undump_table(svc *dynamodb.Client, the_table *Table, filename string) {
	f, err := os.Open(filename)
	if err != nil {
		log.Fatal("failed to open file %s\n%v", filename, err)
	}
	defer f.Close()
	register_gob_types()
	dec := gob.NewDecoder(f)
	items := make([]map[string]types.AttributeValue, 10)
	err = dec.Decode(&items)
	if err != nil {
		log.Fatal("could not decode: ", err)
	}
	for _, row := range items {
		_, err := svc.PutItem(context.TODO(), &dynamodb.PutItemInput{
			TableName: &the_table.real_name,
			Item:      row,
		})
		if err != nil {
			log.Fatalf("failed to add to table %s\n%v", the_table.short_name, err)
		}
	}
}

func register_gob_types() {
	gob.Register(&types.AttributeValueMemberL{})
	gob.Register(&types.AttributeValueMemberN{})
	gob.Register(&types.AttributeValueMemberS{})
	gob.Register(&types.AttributeValueMemberBOOL{})
}

func delete_table_element(svc *dynamodb.Client, the_table *Table, key_value string) {
	items := make(map[string]types.AttributeValue, 1)
	key_field := get_key_field(the_table)
	add_attribute(items, key_field, key_value)
	_, err := svc.DeleteItem(context.TODO(), &dynamodb.DeleteItemInput{
		TableName: &the_table.real_name,
		Key:       items,
	})
	if err != nil {
		log.Fatalf("failed to add to table %s\n%w", the_table.short_name, err)
	}
	log.Println("Item deleted")
}

func relocate_devices(svc *dynamodb.Client, the_table *Table, key_value string, params map[string]string) {
	// Build the request with its input parameters
	resp, err := svc.Scan(context.TODO(), &dynamodb.ScanInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to scan table %s\n%v", the_table.short_name, err)
	}
	old := params["old"]
	new := params["new"]
	devices_found := 0
	locations_found := 0
	for _, row := range resp.Items {
		dev := row["device"].(*types.AttributeValueMemberS).Value
		if dev == key_value {
			devices_found++
			if row["location"].(*types.AttributeValueMemberS).Value == old {
				locations_found++
				row["location"] = &types.AttributeValueMemberS{Value:new}
				_, err := svc.PutItem(context.TODO(), &dynamodb.PutItemInput{
					TableName: &the_table.real_name,
					Item:      row,
				})
				if err != nil {
					log.Fatalf("failed to update to table %s\n%v", the_table.short_name, err)
				}
			}
		}
	}
	log.Printf("%d devices match, %d locations updated\n", devices_found, locations_found)
}

func parse_command_line() (the_table *Table, op int, params map[string]string, key_value string) {
	idx := 1
	args := os.Args
	nargs := len(args)

	if idx >= nargs {
		usage("Table name required")
	}
	table_name := args[idx]
	idx++
	for _, t := range tables {
		if table_name == t.short_name {
			the_table = t
			break
		}
		// Allow names starting with "dev_" to ease experimentation without destroying the true tables.
		if strings.HasPrefix(table_name, "dev_") && table_name[4:] == t.short_name {
			the_table = t
			t.short_name = table_name
			t.real_name = "dev_" + t.real_name
			break
		}
	}
	if the_table == nil {
		usage("Invalid table name " + table_name)
	}

	if idx >= nargs {
		usage("Verb required")
	}
	verb := args[idx]
	idx++

	op = NONE
	params = map[string]string{} // For ADD, RELOCATE_IF
	key_value = ""               // For DELETE, GET, RELOCATE_IF
	switch verb {
	case "create-table":
		op = CREATE_TABLE

	case "delete-table":
		op = DELETE_TABLE

	case "info":
		op = INFO

	case "list", "scan":
		op = SCAN

	case "add":
		op = ADD

		// Populate "params" with field names and field values, check what we can use the values for
		// their fields.
		for idx < nargs {
			pieces := strings.SplitN(args[idx], "=", 2)
			if len(pieces) != 2 {
				usage("Bad parameter " + args[idx])
			}
			idx++
			var the_field *Field = nil
			for _, f := range the_table.fields {
				if f.name == pieces[0] {
					the_field = f
					break
				}
			}
			if the_field == nil {
				usage("Bad field name " + pieces[0])
			}
			if not_expressible(the_field) {
				usage("This type can't be added (yet): '" + the_field.name + "'")
			}
			// TODO: Check that the value matches the type
			params[pieces[0]] = pieces[1]
		}

		// Check that all required expressible fields have been specified

		for _, f := range the_table.fields {
			if required_but_not_expressible(f) {
				continue
			}
			if !f.opt {
				_, found := params[f.name]
				if !found {
					usage("Missing required field '" + f.name + "'")
				}
			}
		}

	case "get", "dump", "undump", "delete":
		switch verb {
		case "get": op = GET
		case "dump": op = DUMP
		case "undump": op = UNDUMP
		case "delete": op = DELETE
		}

		if idx >= nargs {
			usage(fmt.Sprintf("Require key value for '%s'", verb))
		}
		key_value = args[idx]
		idx++
		// TODO: Check that the value matches the type, for GET

	case "relocate-if":
		op = RELOCATE_IF

		if table_name != "observation" || table_name != "dev_observation" {
			usage("relocate-if requires the table to be 'observation'")
		}
		if idx >= nargs {
			usage("Require key value for 'relocate-if'")
		}
		key_value = args[idx]
		idx++
		// TODO: Check that the value matches the type

		if idx+2 != nargs {
			usage("Require old and new location IDs for 'relocate-if'")
		}
		params["old"] = args[idx]
		params["new"] = args[idx+1]
		idx += 2
	
	default:
		usage("Unknown verb " + verb)
	}

	if idx != nargs {
		usage("Garbage trailing arguments for '" + verb + "'")
	}

	return
}

func type_is_list(ty string) bool {
	return ty == TY_SL
}

func type_is_time(ty string) bool {
	return ty == TY_ST || ty == TY_DT
}

func not_expressible(f *Field) bool {
	// Currently all fields are expressible, but it used to not be so
	return false
}

func required_but_not_expressible(f *Field) bool {
	return !f.opt && not_expressible(f)
}

func add_attribute(attrs map[string]types.AttributeValue, f *Field, val string) {
	if f.ty == TY_S {
		attrs[f.name] = &types.AttributeValueMemberS{Value: val}
	} else if f.ty == TY_N || f.ty == TY_I || f.ty == TY_ST || f.ty == TY_DT {
		attrs[f.name] = &types.AttributeValueMemberN{Value: val}
	} else if f.ty == TY_B {
		attrs[f.name] = &types.AttributeValueMemberBOOL{Value: val == "true"}
	} else if f.ty == TY_SL {
		attrvals := []types.AttributeValue{}
		for _, v := range strings.Split(val, ",") {
			attrvals = append(attrvals, &types.AttributeValueMemberS{Value: v})
		}
		attrs[f.name] = &types.AttributeValueMemberL{Value: attrvals}
	} else {
		panic("Should not happen")
	}
}

// "default default"
func add_defdef_attribute(attrs map[string]types.AttributeValue, f *Field) {
	if f.ty == TY_S {
		attrs[f.name] = &types.AttributeValueMemberS{Value: ""}
	} else if f.ty == TY_N || f.ty == TY_I || f.ty == TY_ST || f.ty == TY_DT {
		attrs[f.name] = &types.AttributeValueMemberN{Value: "0"}
	} else if f.ty == TY_B {
		attrs[f.name] = &types.AttributeValueMemberBOOL{Value: false}
	} else if f.ty == TY_SL {
		attrs[f.name] = &types.AttributeValueMemberL{Value: []types.AttributeValue{}}
	} else {
		log.Fatalf("Should not happen\n%v", f.ty)
	}
}

func get_key_field(t *Table) *Field {
	for _, f := range t.fields {
		if f.name == t.key_name {
			return f
		}
	}
	panic("Should not happen")
}

func usage(msg string) {
	log.Printf("ERROR: %s\n", msg)
	help()
	os.Exit(1)
}

func help() {
	fmt.Fprint(os.Stdout, helptext)
}
