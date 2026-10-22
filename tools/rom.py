#!/usr/bin/env python3
# Xera Source/xuno/Rom.h: as táboas da ROM da CPU de voces do Juno-106 (IC29), tiradas do
# listado comentado de Gordon Pearce (github.com/ErroneousBosh/j106roms, ic29.txt).
# Uso: python3 tools/rom.py ic29.txt > Source/xuno/Rom.h
import re, sys
src = open(sys.argv[1]).read().splitlines()
mem = {}
started = False
for line in src:
    if line.startswith('0A00_portCoeffTbl'): started = True
    if not started: continue
    m = re.match(r'^([0-9a-fA-F]{4}):\s+(.*)$', line)
    if not m: continue
    addr = int(m.group(1), 16)
    toks = m.group(2).split()
    if all(len(t) == 4 for t in toks):     # palabras de 16 bits (big-endian no listado)
        for k, t in enumerate(toks):
            v = int(t, 16)
            # na memoria van en little-endian (LDEAX le L primeiro)
            mem[addr + 2*k] = v & 0xff; mem[addr + 2*k + 1] = v >> 8
    elif all(len(t) == 2 for t in toks):
        for k, t in enumerate(toks): mem[addr + k] = int(t, 16)
def bytes_(a, n): return [mem[a+i] for i in range(n)]
def words(a, n): return [mem[a+2*i] | (mem[a+2*i+1] << 8) for i in range(n)]
def fmt(vals, w, per):
    out = []
    for i in range(0, len(vals), per):
        out.append('        ' + ', '.join(('0x%0*x' % (w, v)) for v in vals[i:i+per]) + ',')
    return '\n'.join(out)
tabs = [
 ('PORTA', 0x0a00, 128, 1, 'portamento: paso por volta en 1/256 de semitón (0 = sen portamento)'),
 ('LFO_DEPTH', 0x0a80, 128, 1, 'profundidade do LFO no DCO (deslizador 0–127 → escala de 8 bits)'),
 ('LFO_DELAY_RAMP', 0x0b30, 8, 2, 'atraso do LFO: subida da profundidade por volta (deslizador >> 4)'),
 ('ENV_ATTACK', 0x0b60, 128, 2, 'ataque: o que se suma á envolvente (14 bits) en cada volta'),
 ('LFO_SPEED', 0x0c60, 128, 2, 'velocidade do LFO: o que se suma ao acumulador (0..$1FFF) en cada volta'),
 ('ENV_DECAY', 0x0d60, 128, 2, 'caída e relaxación: multiplicador 0.16 por volta'),
 ('RAMP_CV', 0x0e60, 104, 2, 'CV da rampa do DCO (DAC de 12 bits): a pendente do integrador por nota'),
 ('CLOCK_DIV', 0x0f30, 104, 2, 'divisor do 8253 por nota (reloxo de 1, 2 ou 4 MHz segundo o rexistro)'),
]
print('// Rom.h — As táboas do programa da CPU de voces do Juno-106 (IC29, µPD7811G), tal como están')
print('// na ROM (as direccións, á dereita). Extraídas do desensamblado comentado de Gordon Pearce')
print('// (github.com/ErroneousBosh/j106roms) por tools/rom.py.')
print('#pragma once')
print('#include <cstdint>\n')
print('namespace xuno::rom {\n')
for name, a, n, sz, desc in tabs:
    vals = bytes_(a, n) if sz == 1 else words(a, n)
    t = 'uint8_t' if sz == 1 else 'uint16_t'
    print('    // $%04X %s' % (a, desc))
    print('    inline constexpr %s %s[%d] = {' % (t, name, n))
    print(fmt(vals, 2 if sz == 1 else 4, 16 if sz == 1 else 8))
    print('    };\n')
print('} // namespace xuno::rom')
