#!/bin/bash
#
# Create some sample data.  Note this clears out all tables!

DBOP=dbop/dbop

echo "Deleting tables"
$DBOP device delete-table
$DBOP location delete-table
$DBOP factor delete-table
$DBOP class delete-table
$DBOP observation delete-table

echo "Please wait"
sleep 10

echo "Creating tables"
$DBOP device create-table
$DBOP location create-table
$DBOP factor create-table
$DBOP class create-table
$DBOP observation create-table

echo "Please wait some more"
sleep 10

echo "Adding"
$DBOP class add class=SnappySense 'description=SnappySense v1.x 3rd gen prototypes'
$DBOP class add class=RaspberryPi 'description=Raspberry Pi, all generations'

$DBOP location add location=ambulatory 'description=Here and there' devices=snp_1_1_no_1,snp_1_0_no_1
$DBOP location add location=takterrassen 'description=Takterrassen i Universitetsgata 1' timezone=Europe/Oslo devices=snp_1_1_no_2
$DBOP location add location=u1_5_hotdesk 'description=Hot-desking omraade, 5 etg, Universitetsgata 1' timezone=Europe/Oslo devices=snp_1_1_no_3

$DBOP factor add factor=temperature 'description=Temperature, degrees C'
$DBOP factor add factor=humidity 'description=Relative humidity, percent'
$DBOP factor add factor=uv 'description=UV index, 0..15'
$DBOP factor add factor=light 'description=Luminous intensity, lux'
$DBOP factor add factor=pressure 'description=Air pressure, Pascal*100'
$DBOP factor add factor=airsensor 'description=Air sensor status'
$DBOP factor add factor=airquality 'description=Air quality index, 1..5'
$DBOP factor add factor=tvoc 'description=Total volatile organic compounds, ppb'
$DBOP factor add factor=co2 'description=Equivalent CO_2, ppm'
$DBOP factor add factor=motion 'description=Motion detected (or not)'
$DBOP factor add factor=noise 'description=Noise level, 1..5'

# Hardware 1.0 devices: 1 was built.  It could not use the MIC due to a conflict with WiFi.
$DBOP device add device=snp_1_0_no_1 class=SnappySense location=ambulatory factors=temperature,humidity,uv,light,pressure,airsensor,airquality,tvoc,co2,motion

# Hardware 1.1 devices: 3 were built.
$DBOP device add device=snp_1_1_no_1 class=SnappySense location=ambulatory factors=temperature,humidity,uv,light,pressure,airsensor,airquality,tvoc,co2,motion,noise
$DBOP device add device=snp_1_1_no_2 class=SnappySense location=takterrassen factors=,temperature,humidity,uv,light,pressure,airsensor,airquality,tvoc,co2,motion,noise
$DBOP device add device=snp_1_1_no_3 class=SnappySense location=u1_5_hotdesk factors=temperature,humidity,uv,light,pressure,airsensor,airquality,tvoc,co2,motion,noise

echo "Listing"
$DBOP class list
$DBOP location list
$DBOP factor list
$DBOP device list

echo "Done"
