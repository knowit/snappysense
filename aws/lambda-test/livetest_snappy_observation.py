# Live test of snappy_observation
#
# This assumes you've set up test data with ../test-code/snappysense-demo-data.sh and that you've run
# and passed both livetest_snappy_data.py and livetest_snappy_startup.py

import snappy_data
import snappy_observation
import random

db = snappy_data.connect()
msg = snappy_observation.handle_observation_event(db, {"message_type": "observation",
                                                       "device": "snp_1_1_no_" + str(random.randint(1,3)),
                                                       "class": "SnappySense",
                                                       "sent": random.randint(100000, 1000000),
                                                       "sequenceno": random.randint(1, 1000),
                                                       "F#temperature": random.randint(150,300)/10.0,
                                                       "F#tvoc": random.randint(200, 2000)},
                                                  None)
print(msg) # Should be empty
items = db.scan(TableName="snappy_observation")["Items"]
for i in items:
    print(i)


                          
