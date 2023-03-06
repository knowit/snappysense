upload: snappy-frontend.zip
	aws lambda update-function-code --function-name snappyStartup --zip-file fileb://`pwd`/snappy-frontend.zip

snappy-frontend.zip: index.html lambda_function.py snappy.js
	rm -f snappy-frontend.zip
	zip snappy-frontend.zip $^
