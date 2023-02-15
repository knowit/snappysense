// -*- indent-tabs-mode: t; tab-width: 2; fill-column: 100 -*-
//
// Simple command-line interface to the SnappySense DynamoDB databases.
//
// This also serves as the (executable) definition of the SnappySense data model.
//
// See `helptext` below for all information about how to run this program.

// Useful links:
//
// https://github.com/aws/aws-sdk-go-v2/tree/main/service/dynamodb
// https://github.com/aws/aws-sdk-go-v2/blob/main/service/dynamodb/types/types.go
// https://github.com/aws/aws-sdk-go-v2/blob/main/service/dynamodb/types/enums.go

package main

import (
	"context"
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

The 'table' is one of 'device', 'location', 'class', 'factor', 'history'

The 'verb' and 'argument' combinations are as follows.

  create-table
    ; Create the table in the database

  delete-table
    ; Delete the table from the database

  list
    ; Lists all records in the table, sorted lexicographically by primary key,
    ; with fields chosen by this program.

  info
    ; Show information about the records in the table, notably the fields required
    ; to create a new record and the name of the keys and so on.

  add fieldname=value fieldname=value ...
    ; Add a new record.  All required fields must be present.  Replaces any record
    ; with the same primary key; does not augment it with new data.  Will add
    ; default values for non-required fields that are not present on command line.
    ; If you need to quote something, do it like this: 'fieldname=value with spaces'

  get value
    ; Looks up the record with the given value for the primary key.

  delete value
    ; Deletes the record with the given value for the primary key.

For example

  dbop location add class=toppen 'description=Takterrassen i U1'
  dbop device list
  dbop device get snp_1_1_no_2
`

// Names in Field are short to make some of the tables below legible
type Field struct {
	name string // `device`, etc
	ty   string // `S`, `N`, `SS`, etc
	hide bool   // true if omitted in a listing
	opt  bool   // True if optional
	def  string // optional default value if optional; even for numbers this is a string
}

type Table struct {
	short_name string // `table`, etc
	real_name  string // `snappy_table`, etc
	key_name   string // primary key field name
	fields     []*Field
}

const (
	TY_S  = "string"
	TY_I  = "int"
	TY_B  = "bool"
	TY_N  = "number"
	TY_SS = "string list"
	TY_ST = "server_timestamp"
	TY_DT = "device_timestamp"
	TY_RL = "list of { \"factor\": " + TY_S + ", \"time\": device_timestamp, \"value\": number }"
)

// For table creation.  The value "5" is from the AWS DynamoDB dashboard, good enough?
const (
	PROVISIONED_READ_CAPACITY_UNITS  = 5
	PROVISIONED_WRITE_CAPACITY_UNITS = 5
)

var tables = []*Table{
	&Table{
		short_name: "device", real_name: "snappy_device", key_name: "device",
		fields: []*Field{
			&Field{name: "device", ty: TY_S},
			// The class shall exist in snappy_class
			&Field{name: "class", ty: TY_S},
			// The location, if not empty, shall exist in snappy_location
			&Field{name: "location", ty: TY_S, opt: true},
			&Field{name: "enabled", ty: TY_B, opt: true, def: "false"},
			&Field{name: "reading_interval", ty: TY_I, opt: true, def: "3600"},
			// Each factor shall exist in snappy_factor
			&Field{name: "factors", ty: TY_SS, opt: true},
		}},

	&Table{
		short_name: "class", real_name: "snappy_class", key_name: "class",
		fields: []*Field{
			&Field{name: "class", ty: TY_S},
			&Field{name: "description", ty: TY_S},
		}},

	&Table{
		short_name: "factor", real_name: "snappy_factor", key_name: "factor",
		fields: []*Field{
			&Field{name: "factor", ty: TY_S},
			&Field{name: "description", ty: TY_S},
		}},

	&Table{
		short_name: "location", real_name: "snappy_location", key_name: "location",
		fields: []*Field{
			&Field{name: "location", ty: TY_S},
			// Each device shall exist in snappy_device
			&Field{name: "devices", ty: TY_SS},
		}},

	&Table{
		short_name: "history", real_name: "snappy_history", key_name: "device",
		fields: []*Field{
			// The device shall exist in snappy_device
			&Field{name: "device", ty: TY_S},
			&Field{name: "last_seen", ty: TY_ST},
			// Each factor named in a reading shall exist in snappy_factor
			&Field{name: "readings", ty: TY_RL},
		}},
}

const (
	// year=$1, month=$2, day=$3, hour=$4, minute=$5, second=$6, weekday=$7
	TIMESTAMP_RE = `^(\d\d\d\d)-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)/((sun)|(mon)|(tue)|(wed)|(thu)|(fri)|(sat))$`
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

	cfg, err := config.LoadDefaultConfig(context.TODO(), config.WithRegion("eu-central-1"))
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

	case GET:
		get_table_element(svc, the_table, key_value)

	case ADD:
		add_table_element(svc, the_table, params)

	case DELETE:
		delete_table_element(svc, the_table, key_value)

	default:
		panic("Should not happen")
	}
}

func show_table_info(the_table *Table) {
	fmt.Printf("Table %s\n", the_table.short_name)
	fmt.Printf("  Primary key '%s'\n", the_table.key_name)
	for _, f := range the_table.fields {
		req := "Required field"
		if f.opt {
			req = "Optional field"
		}
		fmt.Printf("  %s '%s', %s\n", req, f.name, f.ty)
		if f.opt && f.def != "" {
			fmt.Printf("    Default '%s'\n", f.def)
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
}

func delete_table(svc *dynamodb.Client, the_table *Table) {
	_, err := svc.DeleteTable(context.TODO(), &dynamodb.DeleteTableInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to delete table %s\n%v", the_table.short_name, err)
	}
	fmt.Println("Table deleted")
}

func scan_table(svc *dynamodb.Client, the_table *Table) {
	// Build the request with its input parameters
	resp, err := svc.Scan(context.TODO(), &dynamodb.ScanInput{
		TableName: &the_table.real_name,
	})
	if err != nil {
		log.Fatalf("failed to scan table %s\n%v", the_table.short_name, err)
	}
	// TODO: Sort by primary key value
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
	if w, ok := v.(*types.AttributeValueMemberSS); ok {
		return fmt.Sprint(w.Value)
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
	fmt.Printf("%s %s\n", the_table.short_name, format_value(row[the_table.key_name]))
	// TODO: Handle the fact that there can be *other* fields than the ones defined by this program,
	// and we should show those too, with some annotation.
	for _, f := range the_table.fields {
		if !f.hide {
			if v, ok := row[f.name]; ok {
				if f.name != the_table.key_name {
					fmt.Printf("  %s: %v\n", f.name, format_value(v))
				}
			}
		}
	}
}

func add_table_element(svc *dynamodb.Client, the_table *Table, params map[string]string) {
	items := make(map[string]types.AttributeValue, 10)
	for _, f := range the_table.fields {
		if required_but_not_expressible(f) {
			add_defdef_attribute(items, f)
		} else if f.opt {
			if f.def != "" {
				add_attribute(items, f, f.def)
			} else {
				add_defdef_attribute(items, f)
			}
		} else {
			add_attribute(items, f, params[f.name])
		}
	}
	_, err := svc.PutItem(context.TODO(), &dynamodb.PutItemInput{
		TableName: &the_table.real_name,
		Item:      items,
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

func delete_table_element(svc *dynamodb.Client, the_table *Table, key_value string) {
	items := make(map[string]types.AttributeValue, 1)
	key_field := get_key_field(the_table)
	add_attribute(items, key_field, key_value)
	_, err := svc.DeleteItem(context.TODO(), &dynamodb.DeleteItemInput{
		TableName: &the_table.real_name,
		Key:       items,
	})
	if err != nil {
		log.Fatalf("failed to add to table %s\n%v", the_table.short_name, err)
	}
	log.Println("Item deleted")
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
	params = map[string]string{} // For ADD
	key_value = ""               // For DELETE, GET
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

	case "delete":
		op = DELETE

		if idx >= nargs {
			usage("Require key value for 'delete'")
		}
		key_value = args[idx]
		idx++
		// TODO: Check that the value matches the type

	case "get":
		op = GET

		if idx >= nargs {
			usage("Require key value for 'get'")
		}
		key_value = args[idx]
		idx++
		// TODO: Check that the value matches the type

	default:
		usage("Unknown verb " + verb)
	}

	if idx != nargs {
		usage("Garbage trailing arguments for '" + verb + "'")
	}

	return
}

func type_is_list(ty string) bool {
	return ty == TY_SS || ty == TY_RL
}

func type_is_time(ty string) bool {
	return ty == TY_ST || ty == TY_DT
}

func not_expressible(f *Field) bool {
	return type_is_list(f.ty)
}

func required_but_not_expressible(f *Field) bool {
	return !f.opt && not_expressible(f)
}

func add_attribute(attrs map[string]types.AttributeValue, f *Field, val string) {
	if f.ty == TY_S || f.ty == TY_ST || f.ty == TY_DT {
		attrs[f.name] = &types.AttributeValueMemberS{Value: val}
	} else if f.ty == TY_N || f.ty == TY_I {
		attrs[f.name] = &types.AttributeValueMemberN{Value: val}
	} else if f.ty == TY_B {
		attrs[f.name] = &types.AttributeValueMemberBOOL{Value: val == "true"}
	} else {
		panic("Should not happen")
	}
}

// "default default"
func add_defdef_attribute(attrs map[string]types.AttributeValue, f *Field) {
	if f.ty == TY_S || f.ty == TY_ST || f.ty == TY_DT {
		attrs[f.name] = &types.AttributeValueMemberS{Value: ""}
	} else if f.ty == TY_N || f.ty == TY_I {
		attrs[f.name] = &types.AttributeValueMemberN{Value: "0"}
	} else if f.ty == TY_B {
		attrs[f.name] = &types.AttributeValueMemberBOOL{Value: false}
	} else if f.ty == TY_SS {
		// DynamoDB disallows empty string sets
		//attrs[f.name] = &types.AttributeValueMemberSS{Value: []string{}}
	} else if f.ty == TY_RL {
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
