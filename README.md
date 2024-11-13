## build

To use 
[avx-512 instruction](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_mm512_popcnt_epi64),
for pop-count, compile with

```bash
cmake -DPOP_COUNT=AVX512
```

It might not be supported by your cpu.  
lscpu should contain `avx512_vpopcntdq`, but this only mean it compiles correctly, it can still not work properly.

If avx-512 instructions are not supported, you can instead use avx instructions:

```bash
cmake -DPOP_COUNT=AVX
```

If avx is not supported at all, you can use

```bash
cmake -DPOP_COUNT=BASIC
```

The default is 

```bash
cmake -DPOP_COUNT=DETECT
```

which tries to detect the best option for your system based on the available compile options.
The fastest option is AVX512 followed by AVX.

## run

```bash
./IBB-cli [-i <input_file>] [-o <output_file>] [-t <temp_dir>] [-k <layers>] [-p <processor_count>]
```

Layers affects the amount of RAM used. Higher Layers in the implementation use more RAM.

Defaults
* layers k: 5
* processors p: 1
* temp_dir t: directory where output_file o is.

In the temporary directory t, $2 * 2^{Layers-1}$ files will be generated.

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