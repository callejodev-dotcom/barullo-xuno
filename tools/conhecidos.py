#!/usr/bin/env python3
# Xera Source/Conhecidos.h: as pegadas (FNV-1a de 64 bits dos 18 bytes) dos 128 sons de fábrica do
# Juno-106 e o seu nome, e a pegada do banco enteiro. Os sons en si non van no plugin: só serven para
# recoñecelos e amosar o seu nome cando alguén abre un .syx con eles.
#
# Uso: python3 tools/conhecidos.py lib.json > Source/Conhecidos.h
#      (lib.json: os sons de fábrica en SysEx, como os publica KR-106 en tools/preset-gen/lib.json)
import json, sys, re

def normalize(p):
    p = [b & 0x7f for b in p]
    sw1 = p[16]
    r = 2 if sw1 & 4 else 1 if sw1 & 2 else 0
    p[16] = (sw1 & ~7) | (1 << r)
    p[17] &= 0x1f
    return p

def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h

lib = json.load(open(sys.argv[1]))
assert len(lib) == 128
patches = [normalize(e['data']) for e in lib]
names = []
for e in lib:
    n = re.sub(r'^[AB][1-8][1-8]\s+', '', e['name']).strip()
    names.append(n.replace('"', "'"))
bank = 1469598103934665603
for p in patches: bank = fnv(p, bank)

print('// Conhecidos.h — Xerado por tools/conhecidos.py. Non editar a man.')
print('// Os sons de fábrica do Juno-106 só pola súa pegada e o seu nome: para recoñecelos nun .syx.')
print('#pragma once')
print('#include <cstdint>\n')
print('namespace xuno::known {\n')
print('struct PatchName { uint64_t hash; const char* name; };')
print('struct BankName { uint64_t hash; const char* name; const char* title; };\n')
print('inline const PatchName PATCHES[128] = {')
for i, (p, n) in enumerate(zip(patches, names)):
    lab = ('A' if i < 64 else 'B') + str((i % 64) // 8 + 1) + str(i % 8 + 1)
    print('    { 0x%016xULL, "%s" },   // %s' % (fnv(p), n, lab))
print('};\n')
print('inline const BankName BANKS[] = {')
print('    { 0x%016xULL, "FÁBRICA", "Os 128 sons de fábrica do Juno-106" },' % bank)
print('};\n')
print('inline int bankIndex (uint64_t h)')
print('{')
print('    for (int i = 0; i < (int) (sizeof (BANKS) / sizeof (BANKS[0])); ++i) if (BANKS[i].hash == h) return i;')
print('    return -1;')
print('}\n')
print('} // namespace xuno::known')
