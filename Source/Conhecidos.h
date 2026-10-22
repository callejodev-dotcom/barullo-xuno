// Conhecidos.h — Xerado por tools/conhecidos.py. Non editar a man.
// Os sons de fábrica do Juno-106 só pola súa pegada e o seu nome: para recoñecelos nun .syx.
#pragma once
#include <cstdint>

namespace xuno::known {

struct PatchName { uint64_t hash; const char* name; };
struct BankName { uint64_t hash; const char* name; const char* title; };

inline const PatchName PATCHES[128] = {
    { 0x4f82cf315661c343ULL, "Brass Set 1" },   // A11
    { 0x39a6fa68ac626102ULL, "Brass Swell" },   // A12
    { 0x993594c31498275aULL, "Trumpet" },   // A13
    { 0x205b4cd6d2f2d750ULL, "Flutes" },   // A14
    { 0x1bb91458e9e32d2eULL, "Moving Strings" },   // A15
    { 0x352b24e3178d4955ULL, "Brass & Strings" },   // A16
    { 0x0e4124537b4ab607ULL, "Choir" },   // A17
    { 0xfac1b68b78068fc4ULL, "Piano I" },   // A18
    { 0x16ea899e40b62ec8ULL, "Organ I" },   // A21
    { 0x83cd4566c356f5ecULL, "Organ II" },   // A22
    { 0xf9fe7e6769619706ULL, "Combo Organ" },   // A23
    { 0xe0091bde7b4d7ff4ULL, "Calliope" },   // A24
    { 0xf20e6a81ce1b1ec5ULL, "Donald Pluck" },   // A25
    { 0x63b75bbbeeb48833ULL, "Celeste* (1 oct.up)" },   // A26
    { 0x1b038f52c1cf72e1ULL, "Elect. Piano I" },   // A27
    { 0x572f80e386752d6eULL, "Elect. Piano II" },   // A28
    { 0x46671758b284d3b3ULL, "Clock Chimes* (1 oct. up)" },   // A31
    { 0x678dbe4ef2c33d1cULL, "Steel Drums" },   // A32
    { 0x9f4654d85c865032ULL, "Xylophone" },   // A33
    { 0xb9b54992657e9310ULL, "Brass III" },   // A34
    { 0x0e2b1e72fc19446dULL, "Fanfare" },   // A35
    { 0xa69e81116c2cf89eULL, "String III" },   // A36
    { 0xacaa5bf2fb9f95f8ULL, "Pizzicato" },   // A37
    { 0x2757e798fb94ada4ULL, "High Strings" },   // A38
    { 0x6d4c4b5423ad8894ULL, "Bass clarinet" },   // A41
    { 0x1c62ded70659241fULL, "English Horn" },   // A42
    { 0x9d125f2d7de3d40aULL, "Brass Ensemble" },   // A43
    { 0x0122da93e78e6857ULL, "Guitar" },   // A44
    { 0x845488bf720f9784ULL, "Koto" },   // A45
    { 0x994c385a6468680cULL, "Dark Pluck" },   // A46
    { 0x3d92fb3a028496ddULL, "Funky I" },   // A47
    { 0xa62f942158db1f0fULL, "Synth Bass I (unison)" },   // A48
    { 0x14a5dc0273d2c42cULL, "Lead I" },   // A51
    { 0x986e21145d092ac4ULL, "Lead II" },   // A52
    { 0x8839c060e4bf4210ULL, "Lead III" },   // A53
    { 0xbf40d3c741dfb368ULL, "Funky II" },   // A54
    { 0xf1b9a4ae6c347a06ULL, "Synth Bass II" },   // A55
    { 0x2b5cf8f0edaa7a16ULL, "Funky III" },   // A56
    { 0x94fd4e32ea48b2dfULL, "Thud Wah" },   // A57
    { 0xd9ea12bc2eb375dcULL, "Going Up" },   // A58
    { 0xf758e94105b3e246ULL, "Piano II" },   // A61
    { 0x8924c80a0afe6b76ULL, "Clav" },   // A62
    { 0x69be775bb53cab63ULL, "Frontier Organ" },   // A63
    { 0xe38c306b51d6022cULL, "Snare Drum (unison)" },   // A64
    { 0xbf3df4c44cd0f9cfULL, "Tom Toms (unison)" },   // A65
    { 0x3d91192f30c4519aULL, "Timpani (unison)" },   // A66
    { 0x0e3bdbc39a47b647ULL, "Shaker" },   // A67
    { 0x978a69caf58d5a35ULL, "Synth Pad" },   // A68
    { 0xd15b1aab4615164aULL, "Sweep I" },   // A71
    { 0xf5aa79575e86bd89ULL, "Pluck Sweep" },   // A72
    { 0xd2a0a8020e7b074bULL, "Repeater" },   // A73
    { 0x5660b6cff3b8e1b5ULL, "Sweep II" },   // A74
    { 0x136fdad3f9eb76aeULL, "Pluck Bell" },   // A75
    { 0xdb1193498097a19aULL, "Dark Synth Piano" },   // A76
    { 0x83d31ebd546a4315ULL, "Sustainer" },   // A77
    { 0xca878ad651059592ULL, "Wah Release" },   // A78
    { 0xe4a83a530f2ed73dULL, "Gong (play low chords)" },   // A81
    { 0x8131c7020ffa0e49ULL, "Resonance Funk" },   // A82
    { 0xb0f85d6ead8b1603ULL, "Drum Booms* (1 oct. down)" },   // A83
    { 0x82dfb543f1ef2478ULL, "Dust Storm" },   // A84
    { 0x1eea88e49efe8f2bULL, "Rocket Men" },   // A85
    { 0x652fbc1882809baaULL, "Hand Claps" },   // A86
    { 0x3d529486ba1a2107ULL, "FX Sweep" },   // A87
    { 0x14aceedb7633e8e1ULL, "Caverns" },   // A88
    { 0x199de13fe5835f8eULL, "Strings" },   // B11
    { 0x4a1f4f450b9fd9b8ULL, "Violin" },   // B12
    { 0x9929efe858968799ULL, "Chorus Vibes" },   // B13
    { 0x3a9cf979d538d9dcULL, "Organ 1" },   // B14
    { 0xaad21ccf1a567223ULL, "Harpsichord 1" },   // B15
    { 0xd0f4f71ec1531c67ULL, "Recorder" },   // B16
    { 0xbbdc2c5679f94f14ULL, "Perc. Pluck" },   // B17
    { 0xa8bd9ce3c21dd417ULL, "Noise Sweep" },   // B18
    { 0x39d482c9bf00f343ULL, "Space Chimes" },   // B21
    { 0xdfa59642ce68686eULL, "Nylon Guitar" },   // B22
    { 0xcb2f59ab268780cdULL, "Orchestral Pad" },   // B23
    { 0x12e6575e2334402eULL, "Bright Pluck" },   // B24
    { 0x2e816d8632c24aaaULL, "Organ Bell" },   // B25
    { 0x76d431a4b6caf217ULL, "Accordion" },   // B26
    { 0xedf1ab22443284ccULL, "FX Rise 1" },   // B27
    { 0x0026b804f22aad61ULL, "FX Rise 2" },   // B28
    { 0x021a30def8482e1cULL, "Brass" },   // B31
    { 0x0122757745af70a9ULL, "Helicopter" },   // B32
    { 0x68c0a08f5d90278dULL, "Lute" },   // B33
    { 0xa66dc0e9d638c647ULL, "Chorus Funk" },   // B34
    { 0x535a8a91d88385bdULL, "Tomita" },   // B35
    { 0xefdcf354fda8dabdULL, "FX Sweep 1" },   // B36
    { 0xa0910be9a029074dULL, "Sharp Reed" },   // B37
    { 0x8cba167acd42e6d2ULL, "Bass Pluck" },   // B38
    { 0xe7fbce1c7b8f6bebULL, "Resonant Rise" },   // B41
    { 0x6bce2c0083b04d23ULL, "Harpsichord 2" },   // B42
    { 0xa07a7bc77037acd8ULL, "Dark Ensemble" },   // B43
    { 0xc54a705b5866104bULL, "Contact Wah" },   // B44
    { 0xf9e61e96d97f99a6ULL, "Noise Sweep 2" },   // B45
    { 0x9a9339271d77476aULL, "Glassy Wah" },   // B46
    { 0x0e023bd39bac79b7ULL, "Phase Ensemble" },   // B47
    { 0xd6985ee568fde0fcULL, "Chorused Bell" },   // B48
    { 0x3112afcf59bef93dULL, "Clav" },   // B51
    { 0x37ab8c3d5a2922a3ULL, "Organ 2" },   // B52
    { 0x955f570ce8eda500ULL, "Bassoon" },   // B53
    { 0x1a847961228dccf9ULL, "Auto Release Noise Sweep" },   // B54
    { 0x9b14c62574de1582ULL, "Brass Ensemble" },   // B55
    { 0x07777894080db4aeULL, "Ethereal" },   // B56
    { 0x960e117d0a31a38bULL, "Chorus Bell 2" },   // B57
    { 0x3ecfa0a0db4a243fULL, "Blizzard" },   // B58
    { 0x05ef096176112cb8ULL, "E. Piano with Tremolo" },   // B61
    { 0x556be0dec48e81f4ULL, "Clarinet" },   // B62
    { 0x953114df02744c82ULL, "Thunder" },   // B63
    { 0xc69a5cc596bf8e23ULL, "Reedy Organ" },   // B64
    { 0x91c06b6cf309b656ULL, "Flute / Horn" },   // B65
    { 0x4526ae6b3874e3a8ULL, "Toy Rhodes" },   // B66
    { 0x5623998f47b22465ULL, "Surf's Up" },   // B67
    { 0xd896e07fbfd62f07ULL, "OW Bass" },   // B68
    { 0xaf7e5a811048f288ULL, "Piccolo" },   // B71
    { 0xafab371d62dd00f8ULL, "Melodic Taps" },   // B72
    { 0x76b47a994c195880ULL, "Meow Brass" },   // B73
    { 0xbc8f6f05d68693bdULL, "Violin (high)" },   // B74
    { 0x241b88081a77cc91ULL, "High Bells" },   // B75
    { 0x3f7eeedc2b787594ULL, "Rolling Wah" },   // B76
    { 0xe3d01d2bbb5ce1b4ULL, "Ping Bell" },   // B77
    { 0x1c5604a8225d0996ULL, "Brassy Organ" },   // B78
    { 0x7e898b7af707bfc9ULL, "Low Dark Strings" },   // B81
    { 0xb40f9782a6703c45ULL, "Piccolo Trumpet" },   // B82
    { 0xd792deba81699d9fULL, "Cello" },   // B83
    { 0x709daf6f647863a4ULL, "High Strings" },   // B84
    { 0xc4d96b2dff8e1ab7ULL, "Rocket Men" },   // B85
    { 0x3b2a15d2012944afULL, "Forbidden Planet" },   // B86
    { 0x1334cd8fdfd4e067ULL, "Froggy" },   // B87
    { 0x68f65ac45be7d6eeULL, "Owgan" },   // B88
};

inline const BankName BANKS[] = {
    { 0xcdc9cc143734c448ULL, "FÁBRICA", "Os 128 sons de fábrica do Juno-106" },
};

inline int bankIndex (uint64_t h)
{
    for (int i = 0; i < (int) (sizeof (BANKS) / sizeof (BANKS[0])); ++i) if (BANKS[i].hash == h) return i;
    return -1;
}

} // namespace xuno::known
