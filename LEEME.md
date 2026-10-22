**Barullo Xuno** é un sintetizador analóxico de seis voces con DCO, compatible cos sons do Juno-106: le e garda os seus ficheiros .syx e recibe os sons e os cambios de parámetro que manda o teclado polo MIDI. Sen mostras e sen latencia.

O motor emula a máquina chip a chip: o programa das dúas CPU (o asignador de voces e a CPU que calcula as envolventes, o LFO e as tensións de control, coas súas táboas e redondeos), os contadores 8253 que marcan o ton dos DCO, o integrador da rampa, o filtro IR3109, os VCA, o filtro paso alto e o chorus de dous BBD.

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3, CLAP e app | `BarulloXuno-…-mac.pkg` | `BarulloXuno-…-mac.zip` |
| **Windows** (64 bits): VST3, CLAP e app | `BarulloXuno-…-windows-setup.exe` | `BarulloXuno-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2, CLAP e app | — | `BarulloXuno-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Que ten

- **O panel enteiro**: LFO (velocidade e atraso), DCO (rexistro 16'/8'/4', LFO, PWM manual ou co LFO, pulso, serra, sub e ruído), HPF de catro posicións, VCF (corte, resonancia, envolvente + ou −, LFO e teclado), VCA (envolvente ou porta, nivel), ENV (ADSR) e chorus I e II. Cada deslizador é un parámetro do DAW e vai en 128 pasos, coma no orixinal; o seu visor di o que significa (Hz, ms, oitavas, dB…), calculado coas táboas da ROM.
- **Sons**: 128 propios, deseñados de cero para Barullo Xuno e igualados en volume:
  - **A11–A88, clásicos**: as familias dun banco de fábrica dos 80 (metais, cordas, teclas, órganos, baixos, solistas, fondos, efectos e percusión);
  - **B11–B88, Escoita**: baile, synthpop, lo-fi, as texturas da casa (Néboa, Mar de Vigo, Faro…), baixos, solistas (unha gaita, tamén), teclas e efectos.
- **Memoria** de 128 sons que se pode gravar (WRITE e o número; gárdase co proxecto) e **MANUAL** (o son é o do panel, sen memoria), con GRUPO A/B, BANCO 1–8 e NÚMERO 1–8 coma no teclado. A **táboa de sons** amosa os 64 de cada grupo cos seus nomes, e **ESCOITAR** toca un acorde mentres está premido (para probar os sons sen teclado MIDI).
- **Carpeta de bancos**: escolle unha carpeta e todos os .syx de Juno-106 que haxa nela aparecen como bancos. Os 128 sons de fábrica recoñécense pola súa pegada e amósanse co seu nome (os sons non van dentro do plugin). **Abrir** e **gardar** .syx dun son ou dun banco enteiro.
- **Asignación** POLY 1, POLY 2 e unísono (no orixinal, POLY 1 e POLY 2 á vez), coma o asignador do Juno-106; **portamento** e **transposición**.
- **Bender**: a panca (aos lados, ton e filtro; cara arriba, vibrato) cos seus tres mandos de sensibilidade.
- **MIDI**: notas, pedal (CC64), roda de afinación, roda de modulación (CC1, co mando LFO do bender), «all notes off» e cambio de programa = son A11–B88. **SysEx de Roland**: un son ($30, $31) vai ao panel e os cambios de parámetro ($32) móveno ao momento.
- **Pantallas**: as seis voces co nivel e a nota de cada unha, o son, a forma de onda que dá o DCO, a envolvente cos seus tempos e o LFO.
- **Función**:
  - *Notas ao instante* (por defecto): cada nota empeza na súa mostra. Apagado, co tempo do orixinal: as ordes do asignador polo porto serie a 31 250 baudios (un acorde de seis notas tarda uns 6 ms en chegar enteiro e un cambio de son asenta parámetro a parámetro) e o bucle da CPU de 4,2 ms;
  - *Voces con tolerancias*: cada voz co seu filtro, a súa resonancia e o seu volume un pouco distintos, coma nun aparello;
  - *Ruído do chorus*: o sopro dos BBD;
  - *Doce voces*: coma dous Juno-106 xuntos;
  - o aviso das versións novas (unha vez ao día; pódese apagar).

## Como se fixo

- **A CPU de voces** (µPD7811G, `Source/xuno/Cpu.h`): o programa reescrito rutina a rutina desde o desensamblado, coas mesmas variables de 8 e 16 bits. Envolventes de 14 bits que se calculan unha vez por volta (a caída cunha multiplicación de 8×8 que non calcula a parte baixa, coma a ROM), LFO triangular de enteiros co seu atraso en dous tramos, portamento en 1/256 de semitón, o ton de cada voz interpolado entre as táboas de divisores e de CV da rampa, o corte do VCF (corte ± LFO ± bend ± envolvente ± teclado, cos seus desbordamentos) e o PWM. O bucle dura uns 4,23 ms e cada orde de nota fai que volva empezar.
- **O asignador** (`Source/xuno/Assigner.h`): POLY 1, POLY 2 e unísono co mesmo mapa de teclas e as mesmas táboas de voces ca a ROM.
- **O DCO** (`Source/xuno/Voice.h`): un contador do 8253 en modo 3 (a conta nova entra en cada medio ciclo) a 1, 2 ou 4 MHz descarga un integrador; o pulso sae dun comparador coa rampa e o sub dun biestable. O tempo vai en tics de 8 MHz enteiros e os saltos, no seu instante exacto con polyBLEP. O ton ten os mesmos erros de redondeo ca o orixinal (±3 cents) e as notas máis altas perden nivel cando o DAC da rampa satura.
- **O VCF**: catro OTA en serie (IR3109), cada un cun tanh, e a realimentación por un VCA que satura; resolto sen atraso. O corte segue as dúas referencias do axuste do manual de servizo.
- **A placa** (`Source/xuno/Board.h`): o HPF (o realce de graves cos valores do esquema; 236 e 754 Hz), o VCA xeral (±10 dB) e o chorus: dous MN3009 de verdade, cada un co seu reloxo movido polo LFO do chorus en contrafase, mostrando o sinal en cada pulso, cos seus filtros de entrada e de saída.
- Todo vai a 190 476 Hz (8 MHz / 42) e pasa á frecuencia do DAW cun filtro de fase mínima (sen latencia).

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/xuno/Rom.h` | As táboas da ROM da CPU de voces (xeradas por `tools/rom.py`) |
| `Source/xuno/Cpu.h` | A CPU de voces: o bucle, as envolventes, o LFO, os tons e as CV |
| `Source/xuno/Assigner.h` | O asignador de voces |
| `Source/xuno/Voice.h` | DCO (8253 e integrador), VCF e VCA |
| `Source/xuno/Board.h` | HPF, VCA xeral, chorus de BBD e ruído |
| `Source/xuno/Output.h` | Paso á frecuencia do DAW |
| `Source/xuno/Synth.h` | Todo xunto, cos tempos do bucle e do porto serie |
| `Source/xuno/Patch.h` | Os 18 bytes dun son e o SysEx ($30, $31, $32) |
| `Source/Banco.h` | Os 128 sons propios (xerados por `tools/banco.py`) |
| `Source/Conhecidos.h` | As pegadas e os nomes dos sons de fábrica (xeradas por `tools/conhecidos.py`) |
| `Source/Plugin*.cpp` | O plugin e a interface |
| `tools/enginetest.cpp` | Proba do motor sen JUCE: afinación, LFO, envolventes, VCF, velocidade |
| `tools/RenderTest.cpp` | Proba do plugin: SysEx, programas, estado, .syx, asignación, bloques e os 128 sons |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.1.0): o texto en galego, o aviso das versións novas, a CI e os instaladores.

## Probas

- **Afinación**: con La3 a 440 Hz, todo o teclado dentro de ±3 cents (os redondeos das táboas do orixinal).
- **LFO** de 0,04 a 29,5 Hz e **ataque** de 0 a 3,3 s, coma a ROM.
- **VCF**: auto-oscilando, o corte segue as referencias do manual de servizo (248 Hz e 992 Hz).
- **SysEx**: un son e un cambio de parámetro recibidos polo MIDI.
- **Bancos**: gardar un .syx e volvelo abrir dá os mesmos bytes.
- **Asignación**: POLY 1 deixa soar as colas, POLY 2 reutiliza a primeira voz, o unísono acende as seis.
- **Bloques**: con bloques de 512 e de 37 mostras a saída é a mesma, mostra a mostra.
- **Os 128 sons** soan, sen valores non numéricos.
- **Estado**: memoria, son, panel e tamaño da xanela gárdanse e recupéranse iguais.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator.

Na CI van todas en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

A versión sae da liña `project(BarulloXuno VERSION …)` de `CMakeLists.txt`; o texto da versión, de `packaging/NOTAS-VERSION.md`.

## Créditos e licenzas

- **Barullo Xuno é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`): podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e coa mesma licenza. Usa JUCE 8 coa súa licenza AGPLv3.
- O programa das CPU, segundo o desensamblado comentado de Gordon Pearce (github.com/ErroneousBosh/j106roms).
- As medidas dun Juno-106 de verdade publicadas por **Ultramaster KR-106** (Kayrock, GPL-3.0): a curva do corte por riba dos 50 kHz, as do VCA, do sub e do ruído, os valores do realce de graves, o chorus (velocidades, profundidades, atraso central e mestura) e a duración do bucle da CPU. Só os datos; o código de Barullo Xuno é propio.
- Os sons de fábrica recoñécense pola pegada calculada sobre a súa versión publicada en SysEx (KR-106, `tools/preset-gen/lib.json`); non van dentro do plugin.
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
- Juno-106 é unha marca de Roland Corporation. Barullo Xuno non ten ningunha relación con Roland: só fala o mesmo SysEx e quere soar coma el.
