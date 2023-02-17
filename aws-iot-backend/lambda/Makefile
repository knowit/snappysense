upload: snappy.zip
	aws lambda update-function-code --function-name snappySense --zip-file fileb://`pwd`/snappy.zip

snappy.zip: snappy_*.py lambda_function.py
	rm -f snappy.zip
	zip snappy.zip $^
