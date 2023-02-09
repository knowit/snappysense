-*- fill-column: 100 -*-

# SnappySense

## Background

Once upon a time ... there was the idea that we should have some kind of sensor unit that our
consultants could carry with them to their customer sites and that would measure the environmental
conditions at those sites (temperature, humidity, light, noise, and so on) and could report those a
service within Dataplattform so that it would be possible to monitor conditions, compare sites, 
discover dangerous conditions, reduce power usage (maybe), and so on.

See the bottom of this document for pointers to historical documents and some other findings.

A custom device was prototyped, and then prototyped again, and now it is being prototyped for the
third time.

## Third-generation prototype focus and requirements

### Focus

The third-generation device is a custom-designed circuit board with space for a microcontroller,
sensors, a display, a speaker, and battery.

The goals and guidelines for this iteration are (based on brainstorming between Gunnar and Lars T):

* This is an internal project for teaching and learning about IoT, experimenting with technology and UX, 
  and creating awareness of the space
* We also intend for the project to be a useful demo to show prospective customers
* We see this as a long-term effort where hardware, firmware, and back-end software all evolve 
  and improve over time, as driven by the needs of the project and the needs of the people working
  on it
* The devices and their accompanying source code should be of reasonable quality and should be
  operational at all times
* There will be reasonable documentation and meaningful quickstart and how-to documents so that
  those that want to experiment with the devices can be up and running quickly, and can contribute 
  meaningfully
* We'll have a feedback and project/issue tracking system that works (we're using Github)

And a non-goal:

* Unlike previous iterations, we take a limited view of how this can be productized to support 
  actual measurements of indoor environmental factors at customer sites.  (However, other 
  projects within Dataplattform could take that on, so long as it does not conflict with the 
  above goals.)

To keep the complexity of the system down we'll tie ourselves to AWS IoT for the time being, with
Lambda and DynamoDB for the prototype.

### High-level design

At some _location_ there is a _device_ that senses environmental _factors_ that it reports to a
backend somewhere.  At the location there may also be _actuators_ that can cause a change to some of
those factors.  For every factor in a location that has an actuator for that factor there may be an
_ideal_ value; the actuator seeks to move the factor toward that ideal.  The ideal is a function of
zero or more factors (for example the ideal for `light` may be "low" if no `movement` has been
detected in a location for some time).

The goal of the system is perhaps to keep the readings for factors at locations with actuators as
close to the ideal as it can; and absent actuators, to log the readings of factors over time to
allow long-term analysis of the quality of the environment at the location, and perhaps to trigger
alerts if the quality is found to be low.

### Architectural parameters

In order to simplify, the central communication paradigm will be a publish/subscribe model based
around MQTT, and we will tie this into a cloud provider with MQTT/IoT functionality.

There is no sense in limiting ourselves to the custom SnappySense unit, anything that can read a
sensor and talk MQTT will do (or indeed, if it can talk something like LoRa that can lets it connect
to a gateway that in turn talks MQTT), and we should encourage this diversity.  But either way, the
central architecture is based on MQTT.

It's nice if devices that have the juice for it can communicate over WiFi, as WiFi networks will be
ubiquitous in work environments.  This means they have to handle enterprise WiFi networks too.

We'll want it to be possible for the device to receive commands or instructions, even though the
utility of these is a little unclear at present.

On the back-end, something with IoT routing and function-as-a-service / serverless and NoSQL will be
fine for prototyping.  Longer term we probably want an container instance and a relational database.

Data volumes (both in transit and at rest) should be restricted to something "sensible", ie, not an
observation every 15s and not storing everything just because we can.  The device should aggregate
locally or measure somewhat infrequently.  The backend should store some historical data but not
all, only "recent" data.

Longer term we want to consider building digital twins for all the devices, but this is a back-end
consideration and independent (mostly) of the device form factors.

## Second generation

Lost in the mists of time...  Supposedly this was a 3D-printed shell for the first generation
prototype and possibly an ESP32 of some kind.

## First generation

An ESP8266 microcontroller, a bunch of sensors, a breadboard, and a mass of wires, enclosed in a cardboard box.

There exists some firmware and backend functionality that was supposedly for this device, URL TBD.

## Historical documents and artifacts

### Overordnet beskrivelse

Vi skal ha massevis av sensorer som måler "ting" (om inneklima, i utgangspunktet) som diskrete
verdier og rapporterer disse til en server, der de kan prosesseres (aggregeres, vises, trackes,
sammenliknes) ytterligere.  Noen av målingene kan brukes i praksis til å regulere inneklima
automatisk, redusere strømforbruk, justere belysning, slå ned på arbeidsmiljøbrudd, etc.

Slides for et prosjekt fra 2018
[her](https://docs.google.com/presentation/d/1pNtJcxtTt6Bbb4HUHGkJigYjLfQWiu_VPyDU1leiLwc/edit#slide=id.g4f6f66adc2_1_1),
"SnappySense".  Mye motivasjon og noe arkitektur.

Vi har et repo på github [her](https://github.com/knowit/digibygg-poc).  Har ikke funnet veldig mye
eksisterende design utover presentasjonen over, koden, og to
[åpne](https://github.com/knowit/Dataplattform-issues/issues/101)
[issues](https://github.com/knowit/Dataplattform-issues/issues/98) på Dataplattform.  Det ligger
heller ikke noe på noe wiki, som jeg har funnet.

Det er imidlertid noen sensor-ideer rapportert som [issues på
digibygg-poc](https://github.com/knowit/digibygg-poc/issues).

I samtale med Tor kom det opp at for eksempel støy, lysforhold, temperatur, luftkvalitet, og UV
kunne være aktuelle faktorer.

Hvordan tenker vi oss bruksmønsteret for disse dataene?  Sannsyligvis vil vi legge opp til at
målinger dukker opp som events på server og at det er funksjoner som kjøres når det kommer nye data,
at det er dashboards som viser data, at data kan aggregeres over tid, o.l., og kanskje også at alle
rådata lagres uten annet enn evt vask for sammenlikning av arbeidssteder og tracking over tid.

Selv om det ikke er åpenbart at noe skal pushes tilbake til devicet for indikasjon på at "nå har du
det dårlig" så kan dette likevel være en greie å ekseperimentere med - enten et lite LED display
eller noen LEDs som har et trafikklys.

Datavolum, nettverkstrafikk, filtrering og diskaksess kan bli en greie mht kostnad hvis det er mange
nok sensorer med høy nok frekvens.  Se nedenfor.

Eksisterende PoC for en esp32 ([kode
her](https://github.com/knowit/digibygg-poc/blob/master/esp32_bme280/src/main.c)) rapporterer
temperatur, luftfuktighet, og trykk, sammen med sin sensor-id.  Se nedenfor for mer om koden.

Det er ikke bare det at vi ønsker oss målinger, men vi ønsker oss også å eksperimentere med for
eksempel OTA oppdatering av koden på devicene, endring av configurasjon på devicene, fleet
management på andre måter (finne ødelagte devices, sikkert mye annet).  Derfor bør devicene ha
mulighet for dette selv om vi strengt tatt ikke ville trengt å ha det.


### Eksisterende kode

Eksisterende kode: https://github.com/knowit/digibygg-poc, skrevet for en esp32 variant med Mongoose
og MQTT.

Virker halvferdig på device?  Logikken ser grei ut men konfigurasjonen er litt uklar.

Mye server kode her:
- mye greier for å håndtere json osv, ville antatt dette kunne dras inn fra økosystemet?
- koden er fra sept 2018 med kommentar "clean up dependencies later" så kanhende dette var en veldig tidlig prototyp
- virker også som en ganske gammeldags aws konfigurasjon?

En annen versjon for Arduino+Firebase (Google Cloud) her:
https://github.com/petterlovereide/froggy-sense/blob/master/code/code.ino

