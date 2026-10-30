**Barullo Xuno** é un sintetizador analóxico de seis voces con DCO, compatible cos sons do Juno-106 (le e garda os seus .syx e recibe o seu SysEx), emulado chip a chip: as dúas CPU, os contadores dos DCO, o filtro IR3109 e o chorus de dous BBD.

Unha ferramenta de [Escoita](https://www.escoita.link).

## Novidades na 0.1.1

- **Limitador de seguridade novo**: mira 1,5 ms por diante e baixa suave antes do pico, en vez de achatalo. Aínda que o son vaia moi forte, xa non soa a clip. O DAW compensa só eses 1,5 ms (a latencia que declara o plugin).
- Tocando en directo, son 1,5 ms máis (non se nota).

## Novidades na 0.1.0

- A primeira versión: o panel enteiro (LFO, DCO, HPF, VCF, VCA, ENV e chorus), 128 sons propios (A, clásicos; B, Escoita), memoria de 128 que se pode gravar, carpeta de bancos .syx (os sons de fábrica recoñécense e amósanse co seu nome), POLY 1, POLY 2 e unísono, portamento, transposición, bender e MIDI con SysEx de Roland.
- Función: notas ao instante ou co tempo do orixinal, voces con tolerancias, ruído do chorus e doce voces.
- O portamento e a asignación (POLY 1, POLY 2, unísono) gárdanse con cada son da memoria e do banco propio.
- Presets propios (`.bxpreset`, gardan todo), A/B para comparar e desfacer e refacer (cada clic é un paso).
- Limitador de seguridade: a saída nunca pasa de −1 dBFS.

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+) — AU, VST3, CLAP e app | `BarulloXuno-…-mac.pkg` | `BarulloXuno-…-mac.zip` |
| **Windows** (64 bits) — VST3, CLAP e app | `BarulloXuno-…-windows-setup.exe` | `BarulloXuno-…-windows-x64.zip` |
| **Linux** (x86_64) — VST3, LV2, CLAP e app | — | `BarulloXuno-…-linux-x86_64.tar.gz` → `./instalar.sh` |

## Se o sistema avisa ao abrilo

- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.
