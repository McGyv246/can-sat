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

In generale in cordice sorgente da mettere sull'arduino si trova in `src/`. Le cartelle al suo interno seguono i nomi assegnati nel documento del progetto. Alla radice della cartella `src/` DEVE esserci il file `main.cpp` che è il file che PlatformIO utilizza per l'Arduino.

Nella cartella `include/` devono esserci i file `.h` necessari a compilare il programma principale SENZA essere messi in sottocartelle (devono stare alla radice di tale cartella). I file `.h` che servono solo per i test o comunque che non devono essere compilati in automatico da PlatformIO li ho lasciati nella cartella dove c'è il file sorgente. Questo farà sì che l'intellisense di VSCode con PlatformIO attivo pensi che ci siano degli errori in caso si includano tali cartelle, ma compilando con la linea di comando non ci sarà questo problema. In caso dia fastidio si potranno spostare tutte in questa specifica cartella.

PlatformIO crea anche una cartella `lib/` ma non dovrebbe servirci.

Nella cartella `mocks/` ci sono i file mock scritti da noi.

Nella cartella `test/` ci andranno tutti i file dei test. I file dei test sarà comodo non metterli in sottocartelle. Mentre eventuali file di utilità andranno messi in sottocartelle. La cartella `test/unity/` contiene il "framework" di testing e non va toccata.

## Esecuzione codice

Sarà possibile eseguire il codice solamente su Arduino IDE. Per farlo funzionare bene in futuro sarà necessario guardare un po' come far lavorare assieme Arduino IDE e le cose scritte in C separatamente.

L'unico modo per eseguire il programma senza l'arduino sarà tramite i test.

## Comandi di debug via Serial

Servono per i test al banco (es. forzare l'espulsione senza un lancio vero). Sono implementati in `src/debug/debug_console.cpp` e sono attivi solo se compilati con `-DSERIAL_DEBUG` o `-DSERIAL_DEBUG_COMMANDS` in `platformio.ini`: **vanno tolti per il volo**. Si scrive un comando per riga nel monitor seriale (115200 baud) e si preme invio.

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

Valori di `N` per `t<N>` (`task_core0_id_t` in `tasks.h`): 0 GPS, 1 IMU, 2 ENV, 3 GAS, 4 UV_FAST, 5 UV_SLOW, 6 FOTORES_FAST, 7 FOTORES_SLOW, 8 BATTERY, 9 MISSION_STATE.

Valori di `N` per `T<N>` (`task_core1_id_t` in `tasks.h`): 0 SD, 1 LORA.

Note:

- I valori simulati con `l` e `u` restano finché non si annullano: i task dei sensori veri non li sovrascrivono.
- Al cambio di stato di volo viene riapplicato il profilo dei task di quello stato, quindi le accensioni/spegnimenti fatti con `t` e `T` si perdono.
- Se è attivo anche `-DBENCH_TEST` (test dei singoli sensori, vedi `bench_setup()` e `bench_setup1()` in `src/bench/bench_setup.cpp`) i profili dei task non vengono mai applicati: `s<N>` cambia lo stato ma i task restano quelli accesi in `bench_setup()`/`bench_setup1()` o con `t` e `T`. Anche `-DBENCH_TEST` va tolto per il volo.
- `d`, `d1` e `d0` esistono solo se è attivo anche `SERIAL_DEBUG_DATA` (già incluso in `-DSERIAL_DEBUG`).
- Ogni riga stampata inizia con tipo, tempo e core, ad esempio `[ERR 12345ms c1] task SD failed` (`ERR` errore, `INF` evento, `DAT` dump dei dati).


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
