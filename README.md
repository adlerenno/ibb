## build

```bash
cc bwt.c data.c tpool.c values.c main.c -o bwt[.exe]
```

## run

```bash
./bwt[.exe] 
```

Unterschützt aktuell keine Angabe von Datei oder layer Anzahl, dies muss in bwt.c/main geändert werden

# Benötigte Strukturen

## Datei

Dies muss von folgendem Format sein:

```
>SequenceX
ACGTACGATGTATGCTAGTCGTACGTAGTCTGATGCGTAGTCGTACGTAGTCGTAGTCCATTGCAGTCATGC
>SequenceX+1
ACTGCAGT
```

Allgemein:

```regexp
((^>.*$) | (^[ACGT]+$))*
```

Kommentarzeilen beginnen mit einem > und enden mit dem Zeilenumbruch  
Datenzeilen enthalten nur A, C, G oder T und enden mit einem Zeilenumbruch

## Filesystem

Das Programm benötigt einen `tmp`-dir im Ausführungsverzeichnis. In diesen werden die $2 * 2^{Layers-1}$ Temp-Dateien
erstellt und genutzt.