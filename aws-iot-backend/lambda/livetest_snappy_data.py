# Live test of snappy_data
# This assumes you've set up test data with ../snappysense-demo-data.sh

import snappy_data

db = snappy_data.connect()

loc = snappy_data.get_location(db, "takterrassen")
print(snappy_data.location_description(loc))
print(snappy_data.location_devices(loc))
print(snappy_data.location_timezone(loc))

dev = snappy_data.get_device(db, "snp_1_1_no_2")
print(snappy_data.device_class(dev))
print(snappy_data.device_location(dev))
print(snappy_data.device_last_contact(dev))
print(snappy_data.device_enabled(dev))
print(snappy_data.device_reading_interval(dev))
print(snappy_data.device_factors(dev))

cls = snappy_data.get_class(db, "SnappySense")
print(snappy_data.class_description(cls))

fac = snappy_data.get_factor(db, "temperature")
print(snappy_data.factor_description(fac))

