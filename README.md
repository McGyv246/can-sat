# CanSat

## Lista strumenti necessari

- gcc
- g++
- make

Se si sta utilizzando windows consiglio di installare WSL e di installare questi programmi all'interno di WSL. Altrimenti buona fortuna a installare gcc su windows (non sono mai riuscito a farlo funzionare).

Per installare WSL aprire una finestra PowerShell con privilegi di amministrazione e inserire il comando:

```

```

## Struttura

La struttura potrà essere cambiata in caso di necessità. (Aggiornare)

can-sat
├───host        -> Contiene i file legati alla macchina su cui si programma
│   ├───mocks
│   └───test
└───src         -> Contiene i file da mettere sull'arduino nel CanSat
    ├───codec
    ├───config
    ├───driver
    ├───hal
    └───service

## Test

I test utilizzano [Unity](https://www.throwtheswitch.org/unity) con Make (inserire comando per i test).