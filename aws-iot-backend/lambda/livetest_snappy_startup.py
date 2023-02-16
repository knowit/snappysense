# Live test of the main snappysense event handler
#
# This assumes you've set up test data with ../snappysense-demo-data.sh and that you've run
# and passed livetest_snappy_data.py, livetest_snappy_startup.py, and livetest_snappy_reading.py

import snappy_data
import snappy_startup

db = snappy_data.connect()
dev = snappy_data.get_device(db, "snp_1_1_no_1")
print(snappy_data.device_last_contact(dev))
msg = snappy_startup.handle_startup_event(db, {"message_type": "startup",
                                               "device": "snp_1_1_no_1",
                                               "class": "SnappySense",
                                               "sent": 12345,
                                               "reading_interval": 5},
                                          None)
print(msg)
# Retrieve again to confirm DB update
dev = snappy_data.get_device(db, "snp_1_1_no_1")
print(snappy_data.device_last_contact(dev))


                          
