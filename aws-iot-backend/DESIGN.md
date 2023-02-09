-*- fill-column: 100 -*-

# SnappySense third-generation prototype, 2022/2023

(Why is not this file the same as the README file?)

This document is about the technical architecture of the backend.  For general background and very
high-level design, see [`../README.md`](../README.md) and [`../BACKGROUND.md`](../BACKGROUND.md).


## Overall architecture

The SnappySense devices perform measurements and cache them for upload.  The possible measurements
are described under the table `FACTOR` in [`DATA-MODEL.md`](DATA-MODEL.md); more can be added, as
other types of sensors become available.

Each SnappySense device is provisioned with individual certificates and keys for AWS IoT (each
device is a "Thing" in AWS IoT), as well as with credentials for a local WiFi network.

The device communicates over WiFi to the AWS IoT MQTT infrastructure (see
[`MQTT-PROTOCOL.md`](MQTT-PROTOCOL.md).  Matching MQTT topics are routed to AWS Lambda.  The Lambda
functions store the data in DynamoDB databases (see [`DATA-MODEL.md`](DATA-MODEL.md)).

There exists (or must exist) a dashboard that allows new devices and locations and other metadata to
be defined in the databases, and allow the measurements stored in the databases to be viewed.

There must also exist various kinds of triggers or alerts: for readings that indicate a very bad
environment; for devices that stop communicating.  These are somehow integrated into the dashboard.

## AWS IoT architecture

## Creating policies and roles for IoT devices


## Policy

### Klient

Hver klient skal ha et sertifikat med en AWS IoT Core Policy som tillater maksimalt:

* `iot:Connect`
* `iot:Subscribe` til `snappy/control/+`
* `iot:Subscribe` til `snappy/command/+`
* `iot:Publish` til `snappy/startup/+/+`
* `iot:Publish` til `snappy/reading/+/+`.

TODO: Aller best ville være om vi kunne kvitte oss med `+` og pub/sub bare kan være med devicets egen klasse
og ID, men det er foreløpig uklart om dette lar seg gjøre i en stor flåte av devices.  Men i tillegg til
sikkerhet er det en annen fordel med en mer restriktiv policy: det blir mye mindre datatrafikk i en stor
flåte av devices.

### Server

En server / lambda skal ha lov å lytte på og publisere til alt, antakelig.

The policy that allows an IoT route to route data to a lambda is created in IAM.  I have one that
looks like this, called `my-iot-policy`:

```
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "lambda:*",
            "Resource": "*"
        }
    ]
}
```


## Creating certs for an IoT device

(This is suitable for a small fleet of devices.  There are other methods.)

Open the AWS Management console as **Dataplattform production** if the cert is going to be
long-lived and not just for testing / prototyping.

Go to AWS IoT.  Under "Manage > Things", ask to create a single thing.

The name of a SnappySense device is `snp_x_y_no_z` where `x.y` is the device type version (1.0 or
1.1 at the time of writing; it's printed on the front of the circuit board) and `z` is the serial
number within that type, usually handwritten on the underside of the processor board.

Make sure **Thing Type** is `SnappySense`.

The thing should have no device shadow, at this point.

On the next screen, auto-generate certificates.

You need to have created an IoT policy for these devices (see separate section).  On the next
screen, pick that policy and then "Create Thing".

You'll be presented with a download screen.  In a new directory, called `snp_x_y_no_z` as above,
store all five files.

## Server

På server mottas meldingene og rutes etter hvert til en lambda.  For prototypen kjører vi dem når meldingen
kommer inn, men i et produksjonsmiljø kan det være attraktivt å bruke en basic-ingest funksjon og
deretter prosessere i batch for å holde kostnadene nede.  TBD.

En lambda for snappy/startup/+/+ leser DEVICE og henter informasjon om devicets konfigurasjon.  Om
dette avviker fra standard (devicet er på) eller det som er rapportert i startup-meldingen sendes en
kontroll-melding til devicet for å konfigurere det.  I tillegg oppdaterer den et felt i HISTORY om
siste kontakt fra devicet (sekunder siden epoch) og skriver evt informasjon om plassering.

En lambda for snappy/reading/+/+ prosesserer måledataene.  Den oppdaterer HISTORY med siste kontakt
og legger til nye verdier i tabellene for sensorene, og forkaster eldste verdier.  Når HISTORY er
oppdatert beregnes ideal-verdiene for hver faktor som ble rapportert på stedet hvor denne sensoren
står.  Hvis ideal-verdien er ulik den leste verdien (kanskje innenfor en faktor som er spesifikk for
hver faktor?) og det er lenge nok siden sist en kommando ble sendt, sendes en action til aktivatoren
for faktoren på stedet, om denne er definert.

TODO: Aggregering over tid?

En ideal-funksjon spesifiseres som en streng, men denne har litt struktur: den er enten et tall (som
representerer ideal-verdien), navnet på en kjent funksjon (som da beregner ideal-verdien fra verdier
den selv har), eller navnet på en kjent funksjon med parametre på formatet
"navn/parameter/parameter".  (Litt uklart hvor komplekst dette bør gjøres.)  En typisk funksjon kan
være "arbeidsdag/22/20", som sier at temperaturen under arbeidsdagen (definert i funksjonen) skal
være 22 grader og under resten av tiden 20 grader.  Men minst like meningsfylt er om disse
parameterverdiene ligger i en tabell et sted og er satt av brukeren.


(Much missing here, needs to be moved)

## Aggregating historical data somehow

- this begs the purpose of gathering the data in the first place
- we want to record "conditions" and monitor them over time and alternatively also provide an
  indicator when "conditions" are bad
- so meaningfully we might consider weekday as the primary key for a datum, as we care more about
  comparing mondays to mondays than january 1 to january 1
- so there may be a table that is indexed by (location, day-of-the-week) and for each entry there is
  a table going back some number of weeks (TBD, but maybe 100?), and in each table element there is
  an object (factor, [{time, reading, ...}, ...])  where the exact fields of the inner structures
  are tbd.  This should be sorted with most recent timestamp first in the inner list.
- there could be a lot of data here if we have one reading per factor per hour, this should be a
  consideration.  In principle, the table could be indexed by a triple, (location, day-of-the-week,
  hour-of-the-day)
- this all makes it easy to report per-location and per-hour and/or per-day-of-the-week.

## API

The sensors, actuators, and backend communicate by MQTT, see mqtt-protocol.md in this directory.

The frontend and the server communicate by REST.  But what is the API?

- obvious first report is to get a plot (say, or even a table) of the reports for a factor for
  a location over time
- so there is some kind of selector for location, a selector for factor, and a selector for a
  date range (could even be "last week", "last month", "last year") and maybe for a time of
  day, this turns into a GET or POST and back comes a page

