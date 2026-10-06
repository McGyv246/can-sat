# CanSat

## Lista strumenti necessari

Per programmare parti che richiedono l'arduino è necessario utilizzare **Arduino IDE**. Se invece si stanno programmando parti che non richiedono l'utilizzo della board io utilizzero Visual Studio Code (VSCode). Lo consiglio perché permette di programmare in modo semplice in puro C. Per le parti legate all'arduino per comodità consiglio di usare sempre VSCode con l'estensione PlatformIO.

Per programmare in C su Windows consiglio l'utilizzo di WSL. WSL è una macchina virtuale che simula Linux all'interno di Windows. Non espone un'interfaccia grafica ma ha solo un terminale. Lo consiglio perché installare programmi come il compilatore per C in windows è molto complicato (non sono mai riuscito a farlo funzionare bene). Mentre se lo si installa all'interno di WSL va utilizzato da linea di comando ma funziona molto bene.

Per installare WSL conviene seguire la [guida ufficiale di Microsoft](https://learn.microsoft.com/it-it/windows/wsl/install).

Per poter programmare in VSCode tramite WSL bisogna seguire [questa guida per il setup](https://code.visualstudio.com/docs/remote/wsl). Una volta completato il setup basterà aprire il terminale di windows (che sia PowerShell o CMD non cambia), navigare alla cartella del progetto e lì eseguire il comando `wsl`. Questo comando aprirà il terminale di WSL all'interno della cartella. In quel terminale basterà eseguire `code .` e verrà aperto VSCode all'interno di WSL.

Gli strumenti necessari, che consiglio di installare all'interno di WSL, sono:

- `gcc`: compilatore C
- `gdb`: debugger C
- `g++`: compilatore C++
- `make`: permette di eseguire comandi che compilano in automatico, eseguono i test ecc.

Per installarli sarà sufficiente eseguire `sudo apt install gcc gdb g++ make`.

Se state leggendo questo probabilmente avete già installato git su Windows. Se volete potete installarlo anche in WSL in modo da utilizzare solo il terminale WSL e non dover usare il terminale Windows. Io lo uso da Windows.

Per VSCode consiglio le seguenti estensioni:

- C/C++ (di microsoft)
- Git Graph (di mhutchie)
- PlatformIO (di platformio.org)

L'estensione C/C++ è importante averla perché è collegata al compilatore e individua errori di codice (altrimenti VSCode dice cose a caso). Inoltre permette di formattare il codice decentemente in automatico quando si salva un file. Oltre a quello permetterebbe di eseguire codice e debuggarlo ma il setup non è così facile e conviene imparare ad utilizzare i comandi da terminale.

Per l'installazione di PlatformIO conviene seguire le istruzioni [questo link](https://docs.platformio.org/en/latest/integration/ide/pioide.html#pioide). Purtroppo con PlatformIO non conviene utilizzare WSL perché bisogna fare un po' di giri per poter vedere il dispositivo USB.

## Struttura

La struttura potrà essere cambiata in caso di necessità. Parte della struttura è fissa per via di come funziona PlatformIO senza configurarlo in modo specifico (cosa che fa perdere tempo e rischia di rompere cose).

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

## Esecuzione codice

Sarà possibile eseguire il codice solamente su Arduino IDE. Per farlo funzionare bene in futuro sarà necessario guardare un po' come far lavorare assieme Arduino IDE e le cose scritte in C separatamente.

L'unico modo per eseguire il programma senza l'arduino sarà tramite i test.

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
| IDLE | GPS agganciato: ha tenuto il fix (almeno `MIN_SATS` satelliti) per `GPS_LOCK_HOLD_MS` nelle ultime 2 ore (`GPS_WARM_VALID_MS`). Di solito è già spento con i satelliti salvati | Verde, due lampi brevi ogni 2 s | Pronto, GPS agganciato |
| IDLE | Task GPS acceso, non ancora agganciato | Azzurro, un lampo breve ogni secondo | Il GPS sta cercando i satelliti (nei primi 10 s è ancora spento, vedi sotto) |
| IDLE | Tutto ok, task GPS spento | Verde, un lampo breve ogni 2 s | Pronto |
| ASCENT, DESCENT | Sempre | Spento | In volo nessuno lo vede, e in ASCENT la sua luce potrebbe far credere ai sensori di luce che c'è stata l'espulsione |
| LANDED | Sempre | Bianco, un lampo breve ogni 2 s | Atterrato, serve per ritrovare il satellite |

Note:

- Gli schemi azzurro e verde doppio compaiono solo se il task GPS è acceso in IDLE (`tasks_enabled_core0_idle` in `src/service/tasks/tasks.c`, acceso di default). Se lo si spegne si vede il lampo verde singolo. Come funziona l'aggancio è spiegato in [GPS: aggancio dei satelliti prima del lancio](#gps-aggancio-dei-satelliti-prima-del-lancio).
- In IDLE i sensori di luce sono accesi e l'espulsione viene rilevata con `EJECTION_LIGHT_HOLD_MS` di luce continua. Per questo in IDLE il LED non resta mai acceso più di 0.3 s di fila (lo controlla `test/led_status_test.c`). La fotoresistenza però può essere lenta a tornare al buio: prima del volo va provato a satellite chiuso, con il LED che lampeggia rosso, che non passi a DESCENT da solo.
- Al banco (env `sat_bench`) lo stato resta BOOT, quindi se è tutto ok il LED è blu fisso. Per vedere gli schemi degli altri stati si usa `s<N>`, ad esempio `s2` per IDLE.

## GPS: aggancio dei satelliti prima del lancio

Da spento il NEO-6M tiene i satelliti trovati nella sua memoria, alimentata dalla batteria tampone sul modulo (anche quando D17 gli toglie l'alimentazione). Quanto ci mette a riagganciarsi dipende da quanto sono vecchi:

- **Effemeridi** (orbite precise, valgono circa 2-4 ore): fix in pochi secondi (hot start).
- **Almanacco** (orbite approssimative, vale settimane): fix in circa 30 s - qualche minuto all'aperto.
- **Niente** (cold start): almeno 12.5 minuti solo per scaricare l'almanacco, molto di più con poco segnale.

Per questo il GPS si prepara in due passi.

**1. Nei giorni prima, a casa (almanacco).** Con l'env `sat_bench`, all'aperto e con il cielo libero, `t0` accende il task GPS. Quando sulla Serial compare `GPS fix found (N sats)` lo si lascia acceso **almeno altri 15 minuti**, così scarica l'almanacco completo, poi si spegne il satellite e si carica l'env `sat`. Ricaricare il firmware non cancella niente: i dati sono nel modulo GPS, non nell'Arduino.

**2. Al campo di lancio (effemeridi), in automatico con l'env `sat`.** Si accende il satellite all'aperto. In IDLE il task GPS (`src/service/gps_warmup.c`):

1. aspetta `GPS_POWER_ON_DELAY_MS` (10 s) prima di accendere il GPS, perché accenderlo insieme al resto all'avvio fa andare in brownout la scheda. Il LED è già azzurro;
2. accende il GPS e aspetta che tenga un fix valido per `GPS_LOCK_HOLD_MS` (60 s): ogni satellite manda le sue effemeridi ogni 30 s, così le salva per tutti quelli visibili. LED azzurro;
3. spegne il GPS: le effemeridi restano salvate e il LED diventa **verde con due lampi**. Ora si può chiudere il satellite;
4. se si resta in IDLE più di `GPS_REFRESH_MS` (1 ora) dall'ultimo aggancio, lo riaccende per aggiornare le effemeridi e poi lo rispegne. Se dentro al razzo non riesce a riagganciarsi resta acceso a cercare. Dopo `GPS_WARM_VALID_MS` (2 ore) senza un aggancio nuovo il LED torna azzurro: al lancio sarà un warm start, più lento ma comunque con un fix durante la discesa.

In ASCENT (e in tutti gli stati dopo) il GPS resta sempre acceso e con le effemeridi salvate ha il fix in pochi secondi. I campioni prima del fix hanno `valid` a false (bit `GPS_OK` a 0) e vanno scartati nell'analisi dei dati.

Sulla Serial (con `SERIAL_DEBUG_INFO`) si vedono `GPS powered on/off`, `GPS fix found/lost (N sats)` e `GPS locked, satellites saved`. Al banco lo stato resta BOOT, quindi `t0` tiene il GPS sempre acceso: per provare il ciclo di IDLE si usa `s2`.

## Stazione di terra: formato dei dati su Serial

Questa sezione è per chi scrive il programma sul PC che legge i dati della stazione di terra (env `ground`, `src/ground/main_ground.cpp`).

### Come funziona

```
Satellite --LoRa--> Arduino di terra --USB Serial--> programma sul PC
```

L'Arduino di terra riceve i pacchetti LoRa (il formato binario `data_to_lora_t` in `include/types.h`), controlla che siano integri, li converte in unità fisiche (°C, Pa, g, ...) e li scrive sulla Serial come **testo**, una riga per ogni evento. Il PC non deve fare nessuna conversione: deve solo leggere le righe, dividerle sulle virgole e usare i numeri.

### Connessione

- Porta seriale USB dell'Arduino (su Windows `COMx`, su Linux `/dev/ttyACMx`), 115200 baud, 8N1. Con la USB dell'RP2040 il baud rate in realtà non conta, ma è meglio impostarlo comunque.
- Testo ASCII, ogni riga finisce con `\r\n`. Conviene togliere sempre `\r\n` (in Python `line.strip()`).
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

Per leggere un bit: `(status >> bit) & 1` (in Python), oppure `status & valore != 0`.

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

Il CRC è CRC-16/MODBUS (polinomio 0xA001 riflesso, valore iniziale 0xFFFF). Un trucco per controllarlo: calcolando il CRC su **tutti** i 55 byte, CRC compreso, il risultato è 0 se il pacchetto è integro.

Decoder in Python di una riga `RAW` (dà gli stessi valori della riga `PKT`):

```python
import struct

LORA_FORMAT = "<HBHIBHhHBh3h3hhBBBHBiihBBHH"  # 55 byte, little-endian, senza padding
LORA_FIELDS = [
    "sync", "version", "seq", "ms_since_boot", "flight_state", "records_dropped",
    "temp_c", "pressure", "humidity_pct", "altitude_baro_m",
    "accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z", "accel_mag",
    "light_uv", "light_fotores", "uv", "gas_raw", "heater_on_s",
    "lat", "lon", "altitude_gps_m", "satfix", "vbatt", "status", "crc16",
]

def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

def decode_raw(hex_str: str) -> dict | None:
    data = bytes.fromhex(hex_str)
    if len(data) != 55 or crc16_modbus(data) != 0:  # CRC su tutto il pacchetto (crc incluso) = 0 se integro
        return None
    r = dict(zip(LORA_FIELDS, struct.unpack(LORA_FORMAT, data)))
    if r["sync"] != 0xA5A5 or r["version"] != 1:
        return None
    return {
        "seq": r["seq"],
        "ms_since_boot": r["ms_since_boot"],
        "flight_state": r["flight_state"],
        "records_dropped": r["records_dropped"],
        "temp_c": r["temp_c"] / 100,
        "pressure_pa": r["pressure"] * 2 + 30000,
        "humidity_pct": r["humidity_pct"] / 2,
        "altitude_baro_m": r["altitude_baro_m"],
        "accel_g": [r[k] / 1000 for k in ("accel_x", "accel_y", "accel_z")],
        "gyro_dps": [r[k] / 10 for k in ("gyro_x", "gyro_y", "gyro_z")],
        "accel_mag_g": r["accel_mag"] / 1000,
        "light_uv": r["light_uv"],
        "light_fotores": r["light_fotores"],
        "uv_index": r["uv"] / 10,
        "gas_raw": r["gas_raw"],
        "heater_on_s": r["heater_on_s"],
        "lat": r["lat"] / 1e7,
        "lon": r["lon"] / 1e7,
        "altitude_gps_m": r["altitude_gps_m"],
        "sats": r["satfix"] >> 3,
        "fix_type": r["satfix"] & 0x07,
        "vbatt_v": r["vbatt"] / 100 + 2.0,
        "status": r["status"],
    }
```

### Esempio di lettura in Python

Con la libreria `pyserial` (`pip install pyserial`). Legge le righe, salva tutto in un file e trasforma le righe `PKT` in dizionari pronti per il grafico:

```python
import serial

PKT_COLUMNS = (
    "rx_ms,rssi_dbm,snr_db,seq,lost,version,ms_since_boot,flight_state,records_dropped,"
    "temp_c,pressure_pa,humidity_pct,altitude_baro_m,"
    "accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,accel_mag_g,"
    "light_uv,light_fotores,uv_index,gas_raw,heater_on_s,"
    "lat,lon,altitude_gps_m,sats,fix_type,vbatt_v,status"
).split(",")

STATUS_ENV_OK = 1 << 0

with serial.Serial("COM5", 115200, timeout=1) as port, open("ground_log.txt", "a") as log:
    while True:
        line = port.readline().decode("ascii", errors="replace").strip()
        if not line:
            continue
        log.write(line + "\n")  # tutto, anche RAW e debug, per poterlo rileggere dopo

        fields = line.split(",")
        if fields[0] == "HDR" and fields[1] == "PKT" and fields[2:] != PKT_COLUMNS:
            print("ATTENZIONE: il formato delle righe PKT è cambiato, aggiornare PKT_COLUMNS")
        elif fields[0] == "PKT" and len(fields) == len(PKT_COLUMNS) + 1:
            pkt = {name: float(value) for name, value in zip(PKT_COLUMNS, fields[1:])}
            status = int(pkt["status"])
            if status & STATUS_ENV_OK:
                print(f"seq {int(pkt['seq'])}: {pkt['temp_c']} °C, {pkt['pressure_pa']} Pa")
        elif fields[0] == "BAD":
            print("pacchetto scartato:", fields[-1])
        # tutte le altre righe (RAW, STAT, debug "[...") qui vengono ignorate
```

### Se il formato cambia

Se si modifica `data_to_lora_t` vanno aggiornati insieme: `ds_to_lora` e `lora_decode` in `src/codec/pack_data_lora.c` (aumentando `LORA_VERSION` in `include/pack_data_lora.h`), la stampa in `src/ground/main_ground.cpp`, questa sezione del README e il programma sul PC. Le colonne nuove vanno aggiunte **in fondo** alla riga `PKT`, così un programma che usa i nomi delle colonne continua a funzionare.


## Test

I test utilizzano [Unity](https://www.throwtheswitch.org/unity). Saranno in futuro creati comandi Make per eseguirli in automatico. Per ora è presente un test di esempio in `test/`, con comandi di compilazione ed esecuzione commentati all'inizio del file.

Da notare che questa cartella di test è considerata da PlatformIO la sua cartella dei suoi test. Noi non la utilizzeremo in questo senso perché tanto non ci servirà mai fare tali test.

## Convenzioni git (importanti)

Ogni volta che si sta facendo un task è necessario creare un ramo di git dedicato. Ad esempio per il setup ho creato il ramo `initial-setup`. Il nome del ramo deve essere tutto minuscolo e contenere solo trattini alti `-` e mai trattini bassi `_`, perché altrimenti git potrebbe avere problemi.

Ricordatevi di farlo perché **non potrete** pushare modifiche direttamente al main (vi darà errore). Per aggiungere modifiche al main sarà necessario fare una pull request. La pull request si fa dal browser (dopo aver pushato le ultime modifiche del proprio ramo) come spiegato [in questo articolo](https://docs.github.com/en/pull-requests/how-tos/create-pull-requests/creating-a-pull-request). Impostate me ed edoardo come reviewer che gli diamo un'occhiata e la mergiamo noi. Se facendo la pull request GitHub dovesse dire che ci sono conflitti, risolveteli.

Se mentre si sta facendo qualcosa si incontra un bug in codice pre-esistente, sarebbe meglio tornare al ramo `main` creare un altro ramo apposito, risolvere il bug e fare una pull request, e mergiare il ramo con il bug sistemato all'interno del proprio ramo principale. Questo serve per risparmiare tempo perché altrimenti se due persone trovano lo stesso bug e lo risolvono, magari in modi diversi, poi ci saranno dei conflitti nel mergiare le modifiche del main (oltre al fatto che è stato fatto il doppio del lavoro per niente).

Per la gestione dei rami il modo più semplice che consiglio è di mergiare il main nel proprio ramo (invece di fare rebase). Questo perché rebase potrebbe creare problemi. Quindi se ci fossero modifiche nuove nel ramo main e si volesse integrarle nel proprio ramo my-branch basterebbe assicurarsi di essere sul proprio ramo (NON sul main), eseguire `git fetch` e poi `git merge origin/main`.

## Convenzioni aggiuntive

### Git

Se volete, per ordine vostro, i rami potete organizzarli mettendo un nome e uno slash davanti. Per esempio io di solito li organizzo così:

- `feat/` (per le aggiunte, da "feature")
- `bug/` (per risolvere i bug)
- `refactor/` (per fare refactoring, cioè riscrivere meglio il codice senza aggiungere nuove funzionalità)
- `chore/` (per le robe che sono una rottura di palle lol)

Quindi ad esempio se volessi aggiungere il driver del GPS chiamerei il ramo `feat/lora-driver`.

Io lo faccio per abitudine siete liberi di non farlo.

### Codice

Per il codice già l'utilizzo dell'estensione per VSCode C/C++ permette di averlo formattato decentemente.

Come linea guida generale evitate i commenti nel codice a meno che non siano strettamente necessari. Di solito il codice si può scrivere dando nomi alle variabili in modo che si spieghi da solo. Inoltre i commenti spesso ci si dimentica di aggiornarli quando si modifica il codice rendendoli fuorvianti.

Per i nomi delle varie cose sarebbe meglio seguire le seguenti convenzioni (che sono abbastanza standard in C):

```
Struct              TitleCase
Campi struct        lowerCase

Funzioni (normali)  snake_case
Funzioni (test)     when_condizioniInizialiTest_expect_risultatoAtteso
Variabili           lowerCase

```

Inoltre chiedo di dare nomi a variabili e funzioni in inglese perché leggere le funzioni built-in di C o di arduino assieme alle proprie scritte in italiano è orribile.

Per i file non c'è convenzione, direi di seguire usare anche qui lo `snake_case`. Per i nomi dei file di test preferirei il formato `nome_cosa_che_si_testa_test`, ad esempio per testare il file `hal_time` si usa il file `hal_time_test`.
