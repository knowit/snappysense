.PHONY: test

test:
	PYTHONPATH=../lambda python3 livetest_snappy_data.py
	PYTHONPATH=../lambda python3 livetest_snappy_startup.py
	PYTHONPATH=../lambda python3 livetest_snappy_observation.py
	PYTHONPATH=../lambda python3 livetest_lambda_function.py
