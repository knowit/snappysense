# Live test of the lambda itself
#
# This assumes you've set up test data with ../snappysense-demo-data.sh and that you've run
# and passed livetest_snappy_data.py

import lambda_function
import random
import snappy_data

db = snappy_data.connect()
dev0 = snappy_data.get_device(db, "snp_1_1_no_1")
print(snappy_data.device_last_contact(dev0))

lambda_function.mock_mqtt = True
lambda_function.snappysense_event({"message_type": "startup",
                                   "device": "snp_1_1_no_1",
                                   "class": "SnappySense",
                                   "sent": 12345,
                                   "interval": 5},
                                  None)
print(lambda_function.mocked_outgoing)
dev1 = snappy_data.get_device(db, "snp_1_1_no_1")
print(snappy_data.device_last_contact(dev1))

items0 = db.scan(TableName="snappy_observation")["Items"]
print(len(items0))
lambda_function.snappysense_event({"message_type": "observation",
                                   "device": "snp_1_1_no_" + str(random.randint(1,3)),
                                   "class": "SnappySense",
                                   "sent": random.randint(100000, 1000000),
                                   "sequenceno": random.randint(1, 1000),
                                   "F#temperature": random.randint(150,300)/10.0,
                                   "F#tvoc": random.randint(200, 2000)},
                                  None)
items1 = db.scan(TableName="snappy_observation")["Items"]
print(len(items1))


                          
