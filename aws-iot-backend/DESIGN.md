-*- fill-column: 100 -*-

# SnappySense 2022/2023

(Dette er fremdeles ganske røfft.)

## Overordnet beskrivelse

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


## Eksisterende kode

Eksisterende kode: https://github.com/knowit/digibygg-poc, skrevet for en esp32 variant med Mongoose
og MQTT.

Virker halvferdig på device?  Logikken ser grei ut men konfigurasjonen er litt uklar.

Mye server kode her:
- mye greier for å håndtere json osv, ville antatt dette kunne dras inn fra økosystemet?
- koden er fra sept 2018 med kommentar "clean up dependencies later" så kanhende dette var en veldig tidlig prototyp
- virker også som en ganske gammeldags aws konfigurasjon?

En annen versjon for Arduino+Firebase (Google Cloud) her:
https://github.com/petterlovereide/froggy-sense/blob/master/code/code.ino

## Hardware

Det er ingen grunn til å begrense seg til én type hardware.

Til PoC i 2018 hadde vi etter sigende en esp32 og noen greier som var lodda sammen.
Det ser ut som om en SnappySense prototyp fra 2019 var en ditto Arduino.

Gunnar har designet en liten sensormodul som kan fungere som ny PoC, bedre enn den gamle.
(TBD: Insert GDrive link here.)

De som har en RPi eller Arduino eller liknende kan jo bare koble en sensor til denne og
sette den ut et sted; alt duger.

## Arkitektur

Vi må definere noen parametre først for hva disse sensorene skal være og hvordan de skal kommunisere.

Hvis det er relativt få sensorer og de kan kobles til strøm kan vi tåle at de er litt dyre og vi kan
basere oss på WiFi for kommunikasjon - de er hjemme hos folk eller på kontorer.  Da er det OK med
MQTT eller HTTP eller annen "tyngre" protokoll direkte på device.  For en prototyp bør dette uansett være greit.

Hvis det blir mange sensorer og de ikke lettt kan kobles til strøm men er i nærheten av tyngre utstyr
(fx, hvis jeg vil ha en på loftet der jeg ikke har el, samt 10 andre rundt i huset) må det bli snakk
om batterier og low-power radio av et eller annet slag, men det er fremdeles mulig å ha en lokal
gateway, som er på strøm og snakker MQTT mot server.

Er de spredt ustrukturert rundt omkring blir det kanskje et LoRaWAN-nettverk, enten via
Things Network (https://www.thethingsnetwork.org/community/oslo/) der det er mulig eller med dedikerte
gateways her og der for å dekke alle områder.  Men selv da vil en gateway sannsynligvis
oversette til MQTT mot sentralisert server.

Datavolum: Et rimelig frekvensinterval for de fleste devicene vi snakker om er kanskje 5 minutter,
og de har da antakelig behov for å sende ganske lite (se nedenfor).  Hvis det blir 200 devicer
blir dette nesten én observasjon pr sekund rapportert inn til serveren.  Hvis det blir
1000 sensorer blir det over tre pr sekund.

Uansett kommer ting inn som events i en funnel hos aws og der vil de bli køet opp og prosessert
på diverse måter av diverse Lambdas (kanskje).  Det kan bli dyrt å kjøre én Lambda per observasjon. 
Bedre kanskje om observasjonene køes opp og håndteres batchvis.

Om en Lambda har behov for å kommunisere ut kan dette gjøres pr MQTT, men mulig det er andre måter,
avhenging av mottaker.

Det har vært snakk om å bygge opp "digitale tvillinger" av sensorene, dette gjøres vel uansett helst
på server hos aws eller liknende.  Tvillingen vil bestå av tilstandsdata, og dersom disse endres av en
lambda eller en melding vil endringen måtte kommuniseres tilbake til sensoren når den er i stand til å
motta dette.  Hvordan dette gjøres er fremdeles uklart for meg.  For tyngre ting og gateways kan MQTT brukes.

Uklart om det er interessant å kommunisere ned til sensoren som sådan.  Kommunikasjonen skal vel
helst være til en kontroller for et fysisk aspekt, dvs lysbryter, varmeregulator, o.l.  Men også
muligheten for å skru en sensor av/på, endre rapporteringsfrekvens, o.l.

Vi kan tenke oss alerts som sier fra om et device slutter å rapportere, om det rapporterer lavt batteri,
om det begynner å sende søppel.

Vi kan tenke oss alerts som sier fra om vi begynner å se data fra devicer vi ikke kjenner.


## Design

Sentralt i modellen står ideen om et forhåndsdefinert sted.  På hvert sted er det et kjent
antall sensorer som måler forskjellige miljøfaktorer, maksimalt én sensor pr faktor,
samt maksimalt én aktuator pr faktor pr sted som kan "gjøre noe" med faktoren.  For hver faktor
for hvert sted er det definert en idealverdi som kan være tidsavhengig men også avhengig av
sensor-verdier for andre faktorer (for eksempel kan idealverdien av faktoren "lys" være "lav"
dersom sensoren for "bevegelse" ikke har slått ut i det siste).

Systemet som helhet har (kanskje) som mål å tilstrebe at hver faktor ligger så nær opp til idealverdien
som er mulig ved å signalisere justeringer til aktuatoren, evt, om det ikke er en aktuator,
å holde styr på hvor langt unna idealverdien faktorene ligger over tid, og evt melde dette
dersom det er problemer som vedvarer.

I praksis er faktorene og kommandospråket til aktuatorene forhåndsbestemt.

Målinger, device-konfigurasjoner, aktuatorkommandoer og alle andre data kommuniseres mellom
device (sensor og aktuator) og server (AWS IoT core, AWS Lambda, …) over MQTT-over-TLS.
Sikkerhet ivaretas av policy-modellen i AWS, dvs, hver device har kun tilgang til data
adressert til det, og kan kun publisere data til server under sitt eget navn.

Siden dette er en prototyp vil en Lambda kjøres hver gang en ny melding ankommer server,
men for et stort meldingsvolum og enkelte faktorer er det bedre å batche prosesseringen.
(For noen faktorer, som lys, vil det være naturlig å unngå batching dersom det finnes
aktuatorer på stedet.


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

