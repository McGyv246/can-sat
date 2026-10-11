# CanSat

Questo repository contiene il codice sviluppato per il nostro CanSat, realizzato per la **Competizione Europea CanSat 2026** organizzata dall'[IEEE Student Branch Brescia](https://ieeesb.unibs.it/).

Un CanSat è un satellite in miniatura, grande quanto una lattina (115 mm × 66 mm, 100–210 g), lanciato da un razzo fino a circa 150–250 m di quota. Dopo il rilascio all'apogeo scende con un paracadute, raccoglie dati e li trasmette alla stazione di terra.

## Obiettivi obbligatori

- Misurare dati sulla presenza di uno o più gas (abbiamo scelto il CO) durante la discesa e trasmetterli in telemetria alla stazione di terra almeno ogni 2 secondi
- Garantire la localizzazione per il recupero (GPS, beacon radio o beeper)
- Rispettare specifici requisiti in attesa del lancio (requisiti di alimentazione e di silenzio radio)
- Rilevare l'espulsione dal razzo e attivare la missione solo dopo il rilascio

## Obiettivi facoltativi scelti da noi

- Ricostruzione della traiettoria di volo
- Identificazione potenziali fonti inquinanti con telecamera termica

## Lista strumenti necessari

L'ambiente di sviluppo consigliato per questo progetto è Visual Studio Code (VS Code) con l'estensione PlatformIO.

Per l'installazione di PlatformIO conviene seguire le istruzioni a [questo link](https://docs.platformio.org/en/latest/integration/ide/pioide.html#pioide).

PlatformIO permette di compilare il programma e flasharlo sull'Arduino, supportando diverse configurazioni predefinite.

Ci sono molte parti di codice che non richiedono l'utilizzo di Arduino e quindi sono state programmate in C. Per la compilazione di queste e l'esecuzione dei test di unità è consigliato utilizzare `gcc` con `make`.

## Struttura delle cartelle

Parte della struttura è così per via di come funziona PlatformIO senza configurarlo in modo specifico (cosa che fa perdere tempo e rischia di rompere cose).

In generale in cordice sorgente da mettere sull'arduino si trova in `src/`. Le cartelle al suo interno seguono i nomi assegnati nel documento del progetto. Nello stesso progetto ci sono più firmware, ognuno è un env di `platformio.ini` che compila solo i suoi file (con `build_src_filter`), quindi c'è un solo `setup()`/`loop()` per firmware:

| Env | Firmware | File principale |
| --- | --- | --- |
| `sat` | Satellite in volo | `src/main.cpp` |
| `sat_bench` (default) | Satellite al banco: solo i sensori accesi in `src/bench/bench_setup.cpp`, con stampe e comandi di debug | `src/main.cpp` |
| `ground` | Stazione di terra, riceve i pacchetti LoRa | `src/ground/main_ground.cpp` |

Si sceglie l'env con `pio run -e <nome>` oppure dal selettore degli env nella barra in basso di VS Code. Un file nuovo in `src/` finisce in automatico in `sat` e `sat_bench`; per usarlo anche in `ground` va aggiunto al suo `build_src_filter`.

Nella cartella `include/` devono esserci i file `.h` necessari a compilare il programma principale SENZA essere messi in sottocartelle (devono stare alla radice di tale cartella). I file `.h` che servono solo per i test o comunque che non devono essere compilati in automatico da PlatformIO li ho lasciati nella cartella dove c'è il file sorgente. Questo farà sì che l'intellisense di VSCode con PlatformIO attivo pensi che ci siano degli errori in caso si includano tali cartelle, ma compilando con la linea di comando non ci sarà questo problema. In caso dia fastidio si potranno spostare tutte in questa specifica cartella.

PlatformIO crea anche una cartella `lib/` ma non dovrebbe servirci.

Nella cartella `mocks/` ci sono i file mock scritti da noi.

Nella cartella `test/` ci andranno tutti i file dei test. I file dei test sarà comodo non metterli in sottocartelle. Mentre eventuali file di utilità andranno messi in sottocartelle. La cartella `test/unity/` contiene il "framework" di testing e non va toccata.

## Esecuzione codice e test

Per compilare e flashare i firmware bisogna selezionare quello desiderato e utilizzare PlatformIO. Il codice quindi eseguirà sull'Arduino.

I test utilizzano [Unity](https://www.throwtheswitch.org/unity).

Per eseguire in automatico i test utilizzare il comando `make test`. Questo comando compila ed esegue tutti i test, per poi in aggiunta stampare un summary con i risultati dei test. Per far sì che questo comando funzioni è necessario che la prima riga di ogni file di test contenga il comando per compilare il test nel formato seguente:

```c
// compile: gcc <arguments>
```

Da notare che questa cartella di test è considerata da PlatformIO la sua cartella dei suoi test. Noi non la utilizzeremo in questo senso perché tanto non ci servirà mai fare tali test.

## Convenzioni codice

Per il codice già l'utilizzo dell'estensione per VSCode C/C++ permette di averlo formattato decentemente.

Convenzioni per i nomi utilizzate:

```
Struct              snake_case_s
Campi struct        snake_case
Tipi di dato        snake_case_t

Funzioni (normali)  snake_case
Funzioni (test)     when_condizioniInizialiTest_expect_risultatoAtteso
Variabili           snake_case

File                snake_case.*
File test           nome_cosa_testata_test.c
```

## Comandi di debug via Serial

Servono per i test al banco (es. forzare l'espulsione senza un lancio vero). Sono implementati in `src/debug/debug_console.cpp` e sono attivi solo se compilati con `-DSERIAL_DEBUG` o `-DSERIAL_DEBUG_COMMANDS` in `platformio.ini` (l'env `sat_bench` li ha già): **per il volo si usa l'env `sat`, che non li ha**. Si scrive un comando per riga nel monitor seriale (115200 baud) e si preme invio.

Le lettere tra `< >` sono dei segnaposto da sostituire con un numero, senza le parentesi e senza spazi: ad esempio `s<N>` si scrive `s4`.

| Comando | Effetto |
| --- | --- |
| `s<N>` | Forza lo stato di volo `N` (vedi tabella sotto). Ad esempio `s4` passa a DESCENT |
| `p` | Stampa subito i due scheduler (core 0 e core 1) |
| `t<N>` | Accende/spegne il task `N` del core 0. Ad esempio `t3` per GAS |
| `T<N>` | Accende/spegne il task `N` del core 1. Ad esempio `T1` per LORA |
| `l1` / `l0` | Simula luce / buio sulla fotoresistenza |
| `l` | Smette di simulare la fotoresistenza |
| `u<X>` | Simula un indice UV pari a `X` (anche decimale, es. `u3.5`). Sopra 2 conta come luce |
| `u` | Smette di simulare l'UV |
| `d` | Stampa subito il datastore, una volta sola |
| `d1` / `d0` | Accende / spegne la stampa periodica del datastore (ogni 2 secondi, spenta all'avvio) |
| `g1` / `g0` | Accende / spegne la stampa delle frasi NMEA grezze del GPS, righe `GPS> $GPGSV,...` (spenta all'avvio, stampa solo mentre il GPS è acceso) |
| `gc` | Cold start del GPS: il modulo cancella dalla sua memoria tampone effemeridi, almanacco, ultima posizione e ora, e ricomincia a cercare da zero (le impostazioni restano). Il GPS deve essere acceso. Il contatore `on` delle righe di debug riparte da 0, così si vede quanto ci mette ad agganciarsi |

Valori di `N` per `s<N>` (`flight_state_t` in `types.h`): 0 BOOT, 1 SELF_TEST, 2 IDLE, 3 ASCENT, 4 DESCENT, 5 LANDED.

Valori di `N` per `t<N>` (`task_core0_id_t` in `tasks.h`): 0 GPS, 1 IMU, 2 ENV, 3 GAS, 4 UV_FAST, 5 UV_SLOW, 6 FOTORES_FAST, 7 FOTORES_SLOW, 8 BATTERY, 9 MISSION_STATE, 10 THERMAL, 11 LED.

Valori di `N` per `T<N>` (`task_core1_id_t` in `tasks.h`): 0 SD, 1 LORA.

Note:

- I valori simulati con `l` e `u` restano finché non si annullano: i task dei sensori veri non li sovrascrivono.
- Al cambio di stato di volo viene riapplicato il profilo dei task di quello stato, quindi le accensioni/spegnimenti fatti con `t` e `T` si perdono.
- Se è attivo anche `-DBENCH_TEST` (test dei singoli sensori, vedi `bench_setup()` e `bench_setup1()` in `src/bench/bench_setup.cpp`) i profili dei task non vengono mai applicati: `s<N>` cambia lo stato ma i task restano quelli accesi in `bench_setup()`/`bench_setup1()` o con `t` e `T`. L'env `sat_bench` ha già `-DBENCH_TEST`, l'env `sat` no.
- `d`, `d1` e `d0` esistono solo se è attivo anche `SERIAL_DEBUG_DATA` (già incluso in `-DSERIAL_DEBUG`).
- Ogni riga stampata inizia con tipo, tempo e core, ad esempio `[ERR 12345ms c1] task SD failed` (`ERR` errore, `INF` evento, `DAT` dump dei dati).

## LED di stato

Il LED RGB (rosso D6, verde D5, blu D4) mostra lo stato del satellite. Lo gestisce il task `LED` del core 0 (`src/service/tasks/led_task.c`), acceso in tutti gli stati di volo. Gli schemi sono definiti in `led_patterns` in `src/service/led_status.c`.

Errori e avvisi si vedono solo a terra prima del lancio (BOOT, SELF_TEST, IDLE). Se ci sono più condizioni vere insieme si vede quella più in alto nella tabella.

| Stato di volo | Condizione | LED | Significato |
| --- | --- | --- | --- |
| BOOT, SELF_TEST, IDLE | Un task ha più di `SCHED_MAX_FAILS` fallimenti, oppure l'ultima scrittura sulla SD è fallita | Rosso, lampeggio veloce (5 volte al secondo) | **Non lanciare.** L'errore di un task resta fino al riavvio |
| BOOT, SELF_TEST, IDLE | Batteria sotto il 10% (`LED_BATTERY_LOW_PCT`) o allarme del sensore della batteria | Giallo, lampeggio (0.3 s acceso, 0.3 s spento) | Batteria scarica |
| BOOT, SELF_TEST, IDLE | L'ultima esecuzione di un task acceso del core 0 è fallita | Viola, lampeggio (0.3 s acceso, 0.3 s spento) | Un sensore non risponde (con il comando `p` si vede quale) |
| BOOT, SELF_TEST | Tutto ok | Blu fisso | Avvio in corso |
| IDLE | Tutto ok | Verde, un lampo breve ogni 2 s | Pronto |
| ASCENT, DESCENT | Sempre | Spento | In volo nessuno lo vede, e in ASCENT la sua luce potrebbe far credere ai sensori di luce che c'è stata l'espulsione |
| LANDED | Sempre | Bianco, un lampo breve ogni 2 s | Atterrato, serve per ritrovare il satellite |

## GPS

Al banco (env `sat_bench`) `t0` accende il task GPS, che tiene il GPS acceso finché il task non viene spento. Sulla Serial (con `SERIAL_DEBUG_INFO`) si vedono `GPS powered on/off` e `GPS fix found/lost (N sats, Xs after power on)`.

### Dati di debug

Finché è acceso, ogni 5 s il task GPS stampa tre righe su interfaccia seriale:

```
GPS on 125s | rx 452B/s bad 0 ovf 0 boots 1 ant OK | mode 1 view 9 used 0 hdop 99.9 time no
GPS snr (PRN:dB-Hz/elev) orbit 3/9 05:31/42 12:0/-- 15:24/17 18:0/-- 21:19/8 ...
GPS hw | jam 4/255 state ok | noise 87 agc 2950/8191 | monitor on | replies 24/25
```

| Campo | Cosa vuol dire | Cosa guardare |
| --- | --- | --- |
| `on` | Secondi da quando è stato acceso | Se torna a 0 da solo la scheda si è riavviata (brownout) |
| `rx` | Byte al secondo arrivati dal GPS | Circa 300-500 è normale. **0: non arriva niente** (alimentazione, D17, fili TX/RX invertiti) |
| `bad` | Frasi NMEA con checksum sbagliato (dall'avvio) | Se cresce: disturbi o baud sbagliato |
| `ovf` | Volte che il buffer di ricezione era pieno (byte persi) | Se cresce il task legge troppo tardi (es. la termocamera blocca il core 0). Fa perdere dati, non l'aggancio |
| `boots` | Volte che il modulo si è avviato (riga `u-blox ag` del suo messaggio di avvio) | Deve restare 1 per ogni accensione. **Se cresce da solo il modulo si riavvia**: la sua alimentazione cala (transistor su D17, picchi di corrente) e riparte ogni volta da capo |
| `ant` | Stato dell'antenna dichiarato dal modulo (`OK`, `OPEN`, `SHORT`, `DONTKNOW`), `?` se non l'ha mai mandato | `OPEN`/`SHORT`: problema con l'antenna |
| `mode` | Tipo di fix (frase GSA): 1 nessuno, 2 2D, 3 3D | Con `mode 3` ma senza `GPS fix found` i dati arrivano in ritardo (più di 2 s) |
| `view` | Satelliti sopra l'orizzonte secondo l'almanacco | 0 o pochi per minuti: niente almanacco, sta ancora facendo il cold start |
| `used` / `hdop` | Satelliti usati nel fix e la sua precisione | Ne servono almeno `MIN_SATS` (4) |
| `time` | Ora UTC nota | `ok` vuol dire che l'ha decodificata da un satellite **oppure** che l'orologio del modulo (tenuto dalla batteria tampone) era già avviato: da sola non prova che riceva qualcosa |
| `snr` | Per ogni satellite in vista: numero, potenza del segnale in dB-Hz (0 = non ricevuto) e, dopo la `/`, elevazione in gradi (`--` = il modulo non ha ancora la sua orbita). `orbit X/Y`: quanti degli Y satelliti hanno l'orbita | **Il dato più utile.** Servono almeno 4 satelliti sopra ~25 per agganciarsi. Tutti 0 o sotto 20: segnale troppo debole (al chiuso, antenna coperta o girata verso il basso, disturbi vicini). Molti satelliti sopra 30 ma `orbit` fermo a 0 per minuti: riceve qualcosa ma non riesce a decodificare i dati, probabile disturbo vicino all'antenna o modulo difettoso. Anche segnali tutti uguali (es. tutti 33-37) e più di ~12 satelliti GPS insieme non sono un cielo vero |

La terza riga è lo stato dell'hardware misurato dal modulo stesso (messaggio UBX-MON-HW, chiesto ogni 5 s; solo su Serial, non viene salvato su SD). Per averlo la scheda manda dei comandi al GPS, quindi serve anche il filo RX del GPS (TX della scheda, D1). Con l'env `sat` (senza `SERIAL_DEBUG_INFO`) non viene mai chiesto.

| Campo | Cosa vuol dire | Cosa guardare |
| --- | --- | --- |
| `jam` | Indicatore di disturbo a frequenza fissa (CW) sulla banda GPS, da 0 (niente) a 255 (fortissimo) | Pochi punti è normale. **Alto e stabile: c'è un disturbo vicino all'antenna** (spegnere/allontanare le cose una alla volta e vedere quando scende) |
| `state` | Giudizio del monitor dei disturbi del modulo: `ok`, `WARNING`, `CRITICAL`, `?` se il monitor è spento | `WARNING`/`CRITICAL`: disturbo |
| `noise` / `agc` | Rumore misurato e quanto il modulo amplifica il segnale d'ingresso (0-8191) | Vanno confrontati tra due prove (es. con e senza USB, al chiuso e all'aperto): se con un disturbo il rumore sale o l'agc scende di molto, è quello |
| `monitor` | Accensione del monitor dei disturbi (comando UBX-CFG-ITFM, rimandato finché il modulo non risponde): `on`, `NAK` (il modulo l'ha rifiutato), `no answer` | `NAK` o `no answer` per sempre: il firmware del modulo non lo supporta (possibile clone). `state` resta `?`, `jam` di solito funziona lo stesso |
| `replies` | Risposte ricevute / richieste mandate dall'accensione | Se non arriva mai una risposta la riga diventa `no MON-HW reply yet`: filo RX del GPS non collegato, oppure modulo che non parla il protocollo UBX (clone) |

Per vedere le frasi NMEA complete si usa il comando `g1` (vedi [Comandi di debug via Serial](#comandi-di-debug-via-serial)).


## Stazione di terra: formato dei dati su Serial

Questa sezione è per chi scrive il programma sul PC che legge i dati della stazione di terra (env `ground`, `src/ground/main_ground.cpp`).

### Come funziona

```
Satellite --LoRa--> Arduino di terra --USB Serial--> programma sul PC
```

L'Arduino di terra riceve i pacchetti LoRa (il formato binario `data_to_lora_t` in `include/types.h`), controlla che siano integri, li converte in unità fisiche (°C, Pa, g, ...) e li scrive sulla Serial come **testo**, una riga per ogni evento. Il PC non deve fare nessuna conversione: deve solo leggere le righe, dividerle sulle virgole e usare i numeri.

### Connessione

- Porta seriale USB dell'Arduino (su Windows `COMx`, su Linux `/dev/ttyACMx`), 115200 baud, 8N1. Con la USB dell'RP2040 il baud rate in realtà non conta, ma è meglio impostarlo comunque.
- Testo ASCII, ogni riga finisce con `\r\n`. Conviene togliere sempre `\r\n`.
- Separatore dei campi: `,` (senza spazi). Decimali con il punto (`21.37`).
- Quando ci si collega a metà trasmissione la prima riga letta può essere tagliata: va scartata se non si riesce a interpretarla.

### Tipi di righe

Il primo campo di ogni riga dice che riga è. **Il programma deve ignorare tutte le righe che non iniziano con uno di questi prefissi**: sulla stessa Serial ci sono anche le stampe di debug, che iniziano con `[` (ad esempio `[INF 1234ms c0] LORA receiver ready`) e in futuro potrebbero aggiungersi altri tipi di riga.

| Prefisso | Quando | Contenuto |
| --- | --- | --- |
| `HDR` | Una volta all'accensione | Nomi delle colonne degli altri tipi di riga |
| `RAW` | Ogni pacchetto ricevuto, anche quelli scartati | I byte del pacchetto così come sono arrivati, in esadecimale |
| `PKT` | Ogni pacchetto integro | I dati del satellite in unità fisiche, più RSSI, SNR e pacchetti persi |
| `BAD` | Ogni pacchetto scartato | Il motivo per cui è stato scartato |
| `STAT` | Ogni 10 secondi | Contatori totali dall'accensione della stazione di terra |

Per ogni pacchetto arrivato viene stampata prima la riga `RAW`, poi `PKT` oppure `BAD`. Se l'errore è della radio (`BAD,...,radio`) non c'è la riga `RAW`, perché i dati non sono leggibili.

Esempio di output:

```
[INF 812ms c0] LORA receiver ready
HDR,PKT,rx_ms,rssi_dbm,snr_db,seq,lost,version,ms_since_boot,flight_state,records_dropped,temp_c,pressure_pa,humidity_pct,altitude_baro_m,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,accel_mag_g,light_uv,light_fotores,uv_index,gas_raw,heater_on_s,lat,lon,altitude_gps_m,sats,fix_type,vbatt_v,status
HDR,BAD,rx_ms,rssi_dbm,snr_db,reason
HDR,STAT,rx_ms,ok,radio_err,bad_length,bad_crc,bad_sync,bad_version,lost
RAW,15230,A5A501010040E201000400005908CC7F5B000000002BFC0000B3040000000000000000000000001749191BE6C585FA000049BB010003F9
PKT,15230,-87.5,9.25,1,0,1,123456,4,0,21.37,95432,45.5,0,0.000,-0.981,0.000,120.3,0.0,0.0,0.000,0,0,0.0,0,0,45.4641943,-9.1896346,0,9,1,3.87,1
BAD,16410,-118.0,-6.75,crc
STAT,20000,1,0,0,1,0,0,0
```

Le righe `HDR` vengono stampate solo all'accensione dell'Arduino: se il programma sul PC si collega dopo, non le vede. **L'ordine delle colonne va quindi scritto anche nel programma** (è quello delle tabelle qui sotto); se arriva una riga `HDR` si può usare per controllare che corrisponda.

### Riga `PKT`

`PKT` seguito da 32 campi, in questo ordine. Le cifre decimali stampate corrispondono alla risoluzione con cui il dato viaggia via LoRa: non ha senso aspettarsi più precisione.

| # | Colonna | Tipo | Unità | Risoluzione | Note |
| --- | --- | --- | --- | --- | --- |
| 1 | `rx_ms` | intero | ms | 1 | Tempo dall'accensione della **stazione di terra** a cui è arrivato il pacchetto |
| 2 | `rssi_dbm` | decimale | dBm | 0.1 | Potenza del segnale ricevuto (più vicino a 0 = più forte, ad esempio -60 forte, -120 al limite) |
| 3 | `snr_db` | decimale | dB | 0.01 | Rapporto segnale/rumore (con LoRa può essere negativo e funzionare comunque) |
| 4 | `seq` | intero | - | 1 | Numero del pacchetto, +1 a ogni invio, dopo 65535 riparte da 0 |
| 5 | `lost` | intero | - | 1 | Pacchetti persi tra il pacchetto integro precedente e questo (vedi sotto) |
| 6 | `version` | intero | - | 1 | Versione del formato del pacchetto, ora sempre 1 |
| 7 | `ms_since_boot` | intero | ms | 1 | Tempo dall'accensione del **satellite** all'ultimo aggiornamento dei dati |
| 8 | `flight_state` | intero | - | 1 | Stato di volo: 0 BOOT, 1 SELF_TEST, 2 IDLE, 3 ASCENT, 4 DESCENT, 5 LANDED |
| 9 | `records_dropped` | intero | - | 1 | Record non scritti sulla SD del satellite perché il buffer era pieno |
| 10 | `temp_c` | decimale | °C | 0.01 | Da -327.68 a 327.67 |
| 11 | `pressure_pa` | intero | Pa | 2 | Da 30000 a 161070 |
| 12 | `humidity_pct` | decimale | % | 0.5 | Da 0 a 127.5 |
| 13 | `altitude_baro_m` | intero | m | 1 | Altitudine dal barometro, relativa alla rampa di lancio |
| 14-16 | `accel_x_g`, `accel_y_g`, `accel_z_g` | decimale | g | 0.001 | Da -32.768 a 32.767 |
| 17-19 | `gyro_x_dps`, `gyro_y_dps`, `gyro_z_dps` | decimale | °/s | 0.1 | Da -3276.8 a 3276.7 |
| 20 | `accel_mag_g` | decimale | g | 0.001 | Modulo dell'accelerazione |
| 21 | `light_uv` | 0/1 | - | - | 1 = il sensore UV vede luce |
| 22 | `light_fotores` | 0/1 | - | - | 1 = la fotoresistenza vede luce |
| 23 | `uv_index` | decimale | - | 0.1 | Indice UV, da 0 a 25.5 |
| 24 | `gas_raw` | intero | conteggi ADC | 1 | Valore grezzo del sensore di gas |
| 25 | `heater_on_s` | intero | s | 1 | Secondi da cui è acceso il riscaldatore del sensore di gas (0 se spento, si ferma a 255) |
| 26 | `lat` | decimale | gradi | 0.0000001 | Latitudine (positiva = nord) |
| 27 | `lon` | decimale | gradi | 0.0000001 | Longitudine (positiva = est) |
| 28 | `altitude_gps_m` | intero | m | 1 | Altitudine dal GPS, sul livello del mare |
| 29 | `sats` | intero | - | 1 | Satelliti usati dal GPS, da 0 a 31 |
| 30 | `fix_type` | intero | - | 1 | Qualità del fix GPS (dalla frase GGA): 0 nessun fix, 1 GPS, 2 DGPS, ... (massimo 7) |
| 31 | `vbatt_v` | decimale | V | 0.01 | Tensione della batteria, da 2.00 a 4.55 |
| 32 | `status` | intero | - | - | Bit di stato dei sensori (vedi sotto), stampato in decimale |

#### Valori validi e valori al limite

- Un valore è affidabile solo se il bit `_OK` del suo sensore in `status` è a 1. Se un sensore non funziona i suoi campi contengono l'ultimo valore letto oppure 0: **vanno ignorati** (ad esempio non disegnati nel grafico).
- I valori fuori dall'intervallo della tabella vengono "schiacciati" al limite: una temperatura di 400 °C arriva come 327.67. Se un valore è esattamente uguale al minimo o al massimo, probabilmente è fuori scala.
- Se sul satellite un valore non è un numero (NaN, ad esempio un sensore che restituisce un errore) arriva come il **minimo** del suo intervallo (ad esempio `temp_c` = -327.68, `pressure_pa` = 30000).
- Finché il GPS non ha un fix (`fix_type` = 0 o bit `GPS_OK` a 0) `lat`, `lon` e `altitude_gps_m` non hanno senso (di solito sono 0).

#### Bit di `status`

| Bit | Valore | Nome | Significato se 1 |
| --- | --- | --- | --- |
| 0 | 1 | `ENV_OK` | Ultima lettura di temperatura/pressione/umidità riuscita |
| 1 | 2 | `IMU_OK` | Ultima lettura di accelerometro/giroscopio riuscita |
| 2 | 4 | `GPS_OK` | Ultima lettura del GPS riuscita |
| 3 | 8 | `UV_OK` | Ultima lettura del sensore UV riuscita |
| 4 | 16 | `FOTORES_OK` | Ultima lettura della fotoresistenza riuscita |
| 5 | 32 | `GAS_OK` | Ultima lettura del sensore di gas riuscita |
| 6 | 64 | `BATTERY_OK` | Ultima lettura della batteria riuscita |
| 7 | 128 | `BATTERY_ALERT` | Batteria scarica (allarme del sensore della batteria) |
| 8 | 256 | `HEATER_ON` | Riscaldatore del sensore di gas acceso |
| 9-15 | - | - | Non usati, sempre 0 |

Sono definiti come `STATUS_*` in `include/pack_data_lora.h`.

#### Pacchetti persi (`lost`)

`lost` è calcolato dalla stazione di terra dalla differenza tra il `seq` di questo pacchetto e quello del pacchetto integro precedente. Ad esempio se arrivano i `seq` 10, 11, 14, la riga del 14 ha `lost` = 2 (il 12 e il 13 sono andati persi o sono arrivati rovinati).

- Il primo pacchetto ricevuto ha sempre `lost` = 0.
- Se il satellite si riavvia `seq` riparte da 0: la stazione se ne accorge perché `ms_since_boot` torna indietro, mette `lost` = 0 e stampa una riga di debug `[INF ...] satellite rebooted`.
- Se anche la stazione di terra si riavvia, i conteggi ripartono da zero.
- Se il satellite salta dei pacchetti perché ha raggiunto il limite di duty cycle incrementa comunque seq, quindi dalla stazione a terra apparirà che alcuni pacchetti sono stati persi.

### Riga `BAD`

```
BAD,<rx_ms>,<rssi_dbm>,<snr_db>,<reason>
```

Le prime tre colonne sono come in `PKT`. `reason` è una di queste parole:

| `reason` | Significato |
| --- | --- |
| `radio` | Il modulo LoRa ha scartato il pacchetto (il suo controllo di integrità è fallito). RSSI e SNR possono essere 0 |
| `length` | Il pacchetto non è lungo 55 byte: probabilmente non è del nostro satellite |
| `crc` | Il CRC non corrisponde: il pacchetto si è rovinato durante la trasmissione |
| `sync` | CRC giusto ma non inizia con `A5A5`: non è un nostro pacchetto |
| `version` | Pacchetto nostro, ma con una versione del formato diversa: satellite e stazione di terra hanno firmware diversi, vanno aggiornati |

Le righe `BAD` servono solo per statistiche e debug: i dati di quei pacchetti non vanno usati.

### Riga `STAT`

```
STAT,<rx_ms>,<ok>,<radio_err>,<bad_length>,<bad_crc>,<bad_sync>,<bad_version>,<lost>
```

Totali dall'accensione della stazione di terra: `ok` pacchetti integri (= righe `PKT`), i successivi sono i conteggi dei vari `reason` delle righe `BAD`, `lost` è la somma di tutti i `lost` delle righe `PKT`. Ad esempio la percentuale di pacchetti ricevuti è `ok / (ok + lost)`.

### Riga `RAW`

```
RAW,<rx_ms>,<byte in esadecimale>
```

Sono i byte del pacchetto esattamente come sono arrivati via radio, 2 cifre esadecimali maiuscole per byte, senza separatori (un pacchetto valido è lungo 55 byte = 110 caratteri). **Conviene salvare tutte le righe `RAW` in un file di log**: se si scopre un errore nella conversione si possono decodificare di nuovo tutti i dati del volo.

Il pacchetto è la struct `data_to_lora_t` di `include/types.h`: little-endian (byte meno significativo per primo), senza spazi tra i campi. `fisico` è il valore nelle unità della tabella di `PKT`, `grezzo` è il numero intero nel pacchetto.

| Offset | Byte | Tipo | Campo | Conversione |
| --- | --- | --- | --- | --- |
| 0 | 2 | uint16 | `sync` | Sempre `0xA5A5` |
| 2 | 1 | uint8 | `version` | - |
| 3 | 2 | uint16 | `seq` | - |
| 5 | 4 | uint32 | `ms_since_boot` | - |
| 9 | 1 | uint8 | `flight_state` | - |
| 10 | 2 | uint16 | `records_dropped` | - |
| 12 | 2 | int16 | `temp_c` | fisico = grezzo / 100 |
| 14 | 2 | uint16 | `pressure` | fisico = grezzo * 2 + 30000 |
| 16 | 1 | uint8 | `humidity_pct` | fisico = grezzo / 2 |
| 17 | 2 | int16 | `altitude_baro_m` | - |
| 19 | 6 | int16 x3 | `accel` (x, y, z) | fisico = grezzo / 1000 |
| 25 | 6 | int16 x3 | `gyro` (x, y, z) | fisico = grezzo / 10 |
| 31 | 2 | int16 | `accel_mag` | fisico = grezzo / 1000 |
| 33 | 1 | uint8 | `is_there_light_uv` | - |
| 34 | 1 | uint8 | `is_there_light_fotores` | - |
| 35 | 1 | uint8 | `uv` | fisico = grezzo / 10 |
| 36 | 2 | uint16 | `gas_raw` | - |
| 38 | 1 | uint8 | `heater_on_since` | - |
| 39 | 4 | int32 | `lat` | fisico = grezzo / 10000000 |
| 43 | 4 | int32 | `lon` | fisico = grezzo / 10000000 |
| 47 | 2 | int16 | `altitude_gps_m` | - |
| 49 | 1 | uint8 | `satfix` | `sats` = grezzo >> 3, `fix_type` = grezzo & 7 |
| 50 | 1 | uint8 | `vbatt` | fisico = grezzo / 100 + 2 |
| 51 | 2 | uint16 | `status` | - |
| 53 | 2 | uint16 | `crc16` | CRC-16/MODBUS dei byte da 0 a 52 |

### Se il formato cambia

Se si modifica `data_to_lora_t` vanno aggiornati insieme: `ds_to_lora` e `lora_decode` in `src/codec/pack_data_lora.c` (aumentando `LORA_VERSION` in `include/pack_data_lora.h`), la stampa in `src/ground/main_ground.cpp`, questa sezione del README e il programma sul PC. Le colonne nuove vanno aggiunte **in fondo** alla riga `PKT`, così un programma che usa i nomi delle colonne continua a funzionare.
