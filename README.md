## build

```bash
cc bwt.c data.c tpool.c popcount.c main.c -o bwt -march=native -O3
```

It's using an avx-512
[instruction](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_mm512_popcnt_epi64),
so it might not be supported by your cpu.  
lscpu should contain `avx512_vpopcntdq`.

Falls avx-512 Instruktionen nicht unterstützt werden, kann `popcount_basic.c` verwendet werden.
Dies benötigt nur avx Instruktionen

```bash
cc bwt.c data.c tpool.c popcount_basic.c main.c -o bwt -mavx -O3
```

Falls avx überhaupt nicht unterstützt werden gibt es noch folgende Möglichkeit.

```bash
cc bwt.c data.c tpool.c values.c main.c -o bwt -O3
```

## run

```bash
./bwt[.exe] FILENAME [LAYERS]
```

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

Das Programm erzeugt einen `tmp`-dir im Ausführungsverzeichnis. In diesen werden die $2 * 2^{Layers-1}$ Temp-Dateien
erstellt und genutzt.