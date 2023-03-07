# -*- fill-column: 100 -*-
#
# Database operations for snappysense.
#
# See ../MQTT-PROTOCOL.md for a description of the messages.
# See ../DATA-MODEL.md for a description of the databases.

import boto3

# There shall be default values for all optional scalar fields in the data model (../dbop/dbop.go)
# and these values shall agree with the values in that program.  Optional list fields have the empty
# list as type always.

DEFAULT_DEV_INTERVAL = 3600
DEFAULT_DEV_LOCATION = ""
DEFAULT_DEV_LAST_CONTACT = 0
DEFAULT_DEV_ENABLED = True
DEFAULT_LOC_TIMEZONE = ""

# Connect to the database and return the object representing it

def connect():
    return boto3.client('dynamodb', region_name='eu-central-1')

################################################################################
#
# Helpers

# Lookup by string key and return it, return None if not found

def standard_lookup(db, table_name, key_name, key_value):
    probe = db.get_item(TableName=table_name, Key={key_name: {"S": key_value}})
    if probe == None or "Item" not in probe:
        return None
    return probe["Item"]

def string_field(item, name):
    return item[name]["S"]

def opt_string_field(item, name, defval):
    if name in item:
        return item[name]["S"]
    return defval

def opt_int_field(item, name, defval):
    if name in item:
        return int(item[name]["N"])
    return defval

def set_opt_int_field(item, name, val):
    item[name]["N"] = str(val)

def opt_bool_field(item, name, defval):
    if name in item:
        return item[name]["BOOL"]
    return defval

def string_list(item, name):
    ds = []
    if name in item:
        for d in item[name]["L"]:
            ds.append(d["S"])
    return ds

################################################################################
#
# DEVICE

def get_device(db, device_name):
    return standard_lookup(db, "snappy_device", "device", device_name)

def write_device_entry(db, device_entry):
    db.put_item(TableName="snappy_device", Item=device_entry)
    
def device_class(device_entry):
    return string_field(device_entry, "class")

def device_location(device_entry):
    return opt_string_field(device_entry, "location", DEFAULT_DEV_LOCATION)

def device_last_contact(device_entry):
    return opt_int_field(device_entry, "last_contact", DEFAULT_DEV_LAST_CONTACT)

# 'lc' is reliable because it is 'received' time, ie generated on the server
def device_set_last_contact(device_entry, lc):
    set_opt_int_field(device_entry, "last_contact", lc)

def device_enabled(device_entry):
    return opt_bool_field(device_entry, "enabled", DEFAULT_DEV_ENABLED)

def device_interval(device_entry):
    return opt_int_field(device_entry, "interval", DEFAULT_DEV_INTERVAL)

def device_factors(device_entry):
    return string_list(device_entry, "factors")


################################################################################
#
# CLASS
                        
def get_class(db, class_name):
    return standard_lookup(db, "snappy_class", "class", class_name)

def class_description(class_entry):
    return string_field(class_entry, "description")


################################################################################
#
# FACTOR
                        
def get_factor(db, factor_name):
    return standard_lookup(db, "snappy_factor", "factor", factor_name)

def factor_description(factor_entry):
    return string_field(factor_entry, "description")


################################################################################
#
# LOCATION
                        
def get_location(db, location):
    return standard_lookup(db, "snappy_location", "location", location)

def location_description(location_entry):
    return string_field(location_entry, "description")

def location_devices(location_entry):
    return string_list(location_entry, "devices")

def location_timezone(location_entry):
    return opt_string_field(location_entry, "timezone", DEFAULT_LOC_TIMEZONE)


################################################################################
#
# OBSERVATION

def put_observation(db, key, device, location, sequenceno, received, sent, factors):
    record = {"key":        {"S":key},
              "device":     {"S":device},
              "location":   {"S":location},
              "sequenceno": {"N":str(sequenceno)},
              "received":   {"N":str(received)},
              "sent":       {"N":str(sent)}}
    for k in factors:
        record[k] = {"N":str(factors[k])}
    db.put_item(TableName="snappy_observation", Item=record)


