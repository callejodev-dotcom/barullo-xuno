// Banco.h — Xerado por tools/banco.py. Non editar a man.
// Os 128 sons propios de Barullo Xuno: A11–A88 clásicos, B11–B88 Escoita.
#pragma once
#include <cstdint>

namespace xuno::bank {

// o portamento e a asignación van co son (o Juno-106 non os garda): portaOn, porta (0–255) e mode
// (0 POLY 1, 1 POLY 2, 2 unísono)
struct Entry { const char* name; const char* desc; uint8_t p[18]; bool portaOn = false; uint8_t porta = 90; uint8_t mode = 0; };

inline const Entry PATCHES[128] = {
    { "Metais", "Metais de sintetizador: o filtro ábrese co ataque e pecha ata media altura.",
      { 68, 70, 14, 0, 0, 48, 12, 48, 0, 64, 79, 6, 62, 70, 30, 0, 82, 17 } },   // A11
    { "Metais suaves", "Unha sección que entra amodo e incha, para acordes longos.",
      { 0, 0, 0, 0, 0, 44, 8, 40, 0, 50, 69, 42, 80, 90, 45, 0, 82, 17 } },   // A12
    { "Trompeta", "Serra e pulso, ataque rápido e un vibrato que chega tarde.",
      { 72, 64, 24, 30, 0, 54, 18, 42, 0, 80, 20, 5, 55, 64, 22, 0, 122, 17 } },   // A13
    { "Trompas", "Escuras e redondas, en 8' co sub por debaixo.",
      { 0, 0, 0, 0, 0, 38, 6, 30, 0, 60, 75, 22, 70, 88, 40, 34, 82, 17 } },   // A14
    { "Metais 80", "Brillantes e curtos, cun pouco de resonancia: o metal de sintetizador dos 80.",
      { 0, 0, 0, 0, 0, 50, 28, 58, 0, 70, 93, 3, 58, 48, 34, 0, 18, 17 } },   // A15
    { "Fanfarra", "Serra e pulso estreito xuntos, para riffs que corten.",
      { 0, 0, 0, 46, 0, 56, 16, 52, 0, 64, 23, 2, 50, 60, 26, 0, 122, 17 } },   // A16
    { "Sección", "Metais con PWM lento: a sección enteira, ancha.",
      { 24, 0, 0, 56, 0, 50, 10, 40, 0, 60, 29, 12, 70, 80, 40, 0, 90, 16 } },   // A17
    { "Metais e cordas", "Metade metal, metade corda: ataque lento e chorus.",
      { 30, 0, 0, 60, 0, 58, 4, 26, 0, 50, 23, 30, 80, 100, 60, 0, 90, 16 } },   // A18
    { "Cordas", "Conxunto de cordas co chorus I: o son polo que se coñece este sintetizador.",
      { 60, 60, 10, 0, 0, 72, 0, 12, 0, 40, 71, 40, 70, 110, 62, 0, 82, 9 } },   // A21
    { "Cordas lentas", "Entran e saen amodo, para fondos.",
      { 0, 0, 0, 0, 0, 66, 0, 8, 0, 40, 77, 70, 90, 118, 86, 0, 18, 9 } },   // A22
    { "Conxunto", "Serra e PWM: moitas cordas, moi anchas.",
      { 28, 0, 0, 50, 0, 70, 0, 10, 0, 40, 32, 38, 80, 112, 64, 0, 26, 8 } },   // A23
    { "Violíns", "Brillantes e co vibrato atrasado, en 4'.",
      { 74, 56, 22, 0, 0, 80, 0, 10, 0, 50, 64, 30, 70, 110, 50, 0, 84, 9 } },   // A24
    { "Violonchelos", "En 16': corpo e arco.",
      { 66, 60, 14, 0, 0, 52, 6, 20, 0, 40, 78, 28, 70, 100, 50, 0, 81, 17 } },   // A25
    { "Pizzicato", "Cordas punteadas: sen sustain.",
      { 0, 0, 0, 0, 0, 50, 10, 44, 0, 80, 88, 0, 36, 0, 32, 0, 82, 17 } },   // A26
    { "Trémolo", "Cordas co LFO no filtro: un trémolo rápido.",
      { 86, 0, 0, 0, 0, 64, 0, 8, 34, 40, 77, 26, 70, 110, 50, 0, 82, 9 } },   // A27
    { "Cordas brillantes", "Abertas e cun pouco de pulso, para que se oian na mestura.",
      { 34, 0, 0, 40, 0, 88, 0, 6, 0, 30, 30, 34, 70, 110, 58, 0, 26, 8 } },   // A28
    { "Piano", "O piano de sintetizador: serra e pulso, e a caída do filtro.",
      { 0, 0, 0, 20, 0, 40, 6, 50, 0, 100, 36, 0, 74, 0, 46, 0, 90, 17 } },   // A31
    { "Piano eléctrico", "Cadrada e sub, suave: un eléctrico morno.",
      { 80, 0, 6, 0, 0, 48, 0, 30, 0, 90, 53, 0, 80, 20, 46, 40, 74, 17 } },   // A32
    { "Eléctrico brillante", "Pulso estreito: o eléctrico con máis ataque.",
      { 0, 0, 0, 46, 0, 52, 14, 40, 0, 90, 53, 0, 76, 16, 44, 0, 10, 17 } },   // A33
    { "Clavinete", "Pulso estreito, resonancia e o paso alto: funk.",
      { 0, 0, 0, 80, 0, 56, 34, 40, 0, 90, 85, 0, 44, 14, 10, 0, 106, 9 } },   // A34
    { "Clave", "Clavecín en 4', moi aberto e sen sustain.",
      { 0, 0, 0, 30, 0, 66, 0, 36, 0, 80, 46, 0, 56, 0, 40, 0, 28, 1 } },   // A35
    { "Celesta", "Cadrada en 4', doce.",
      { 0, 0, 0, 0, 0, 58, 0, 20, 0, 80, 48, 0, 70, 0, 62, 0, 76, 17 } },   // A36
    { "Vibráfono", "Cadrada co LFO no filtro: o trémolo do vibráfono.",
      { 80, 0, 0, 0, 0, 52, 0, 16, 22, 90, 61, 0, 82, 0, 72, 0, 74, 17 } },   // A37
    { "Marimba", "Cadrada e sub, curta e de madeira.",
      { 0, 0, 0, 0, 0, 36, 32, 54, 0, 100, 73, 0, 40, 0, 36, 50, 106, 17 } },   // A38
    { "Órgano", "Cadrada e sub co VCA en porta: o órgano de sempre, co chorus II.",
      { 0, 0, 0, 0, 0, 100, 0, 0, 0, 0, 31, 0, 0, 127, 4, 90, 10, 21 } },   // A41
    { "Órgano percutido", "Cun golpe do filtro ao empezar cada nota.",
      { 0, 0, 0, 0, 0, 66, 0, 40, 0, 60, 46, 0, 34, 80, 6, 80, 10, 17 } },   // A42
    { "Órgano combo", "Pulso e serra en porta: o órgano dos grupos dos 60.",
      { 0, 0, 0, 60, 0, 94, 0, 0, 0, 0, 17, 0, 0, 127, 4, 0, 90, 21 } },   // A43
    { "Órgano de igrexa", "En 16' e con todo: serra, pulso e sub.",
      { 0, 0, 0, 0, 0, 88, 0, 0, 0, 0, 16, 16, 0, 127, 70, 100, 25, 17 } },   // A44
    { "Calíope", "Pulso estreito, un pouco de aire e vibrato.",
      { 76, 0, 30, 70, 10, 80, 0, 0, 0, 0, 40, 0, 0, 127, 6, 0, 106, 21 } },   // A45
    { "Harmonio", "Serra con PWM lento, coma un fol.",
      { 20, 0, 0, 40, 0, 64, 14, 0, 0, 0, 35, 14, 0, 120, 30, 0, 90, 16 } },   // A46
    { "Acordeón", "Serra e pulso cun vibrato rápido e pequeno.",
      { 96, 0, 10, 30, 0, 74, 8, 0, 0, 0, 27, 6, 0, 120, 16, 0, 26, 17 } },   // A47
    { "Órgano house", "Cadrada e sub co filtro pechando axiña: o órgano das pistas de baile dos 90.",
      { 0, 0, 0, 0, 0, 44, 40, 56, 0, 60, 57, 0, 50, 0, 30, 100, 106, 17 } },   // A48
    { "Baixo", "Serra e sub, co realce de graves.",
      { 0, 0, 0, 0, 0, 34, 20, 48, 0, 40, 32, 0, 56, 40, 10, 100, 114, 25 } },   // A51
    { "Baixo redondo", "Cadrada e todo o sub: un baixo cheo.",
      { 0, 0, 0, 0, 0, 30, 10, 30, 0, 40, 0, 0, 60, 60, 12, 127, 106, 25 } },   // A52
    { "Baixo punteado", "Serra moi curta e con resonancia.",
      { 0, 0, 0, 0, 0, 30, 56, 68, 0, 50, 127, 0, 44, 0, 16, 0, 114, 17 } },   // A53
    { "Baixo funk", "Pulso estreito, sub e un filtro que fala.",
      { 0, 0, 0, 60, 0, 30, 50, 64, 0, 40, 117, 0, 36, 10, 12, 70, 106, 17 } },   // A54
    { "Baixo groso", "Serra, PWM e sub co chorus I: o baixo groso de seis voces.",
      { 26, 0, 0, 50, 0, 40, 14, 36, 0, 40, 14, 0, 60, 60, 14, 90, 90, 24 } },   // A55
    { "Baixo ácido", "Resonancia alta e a envolvente ao fondo.",
      { 0, 0, 0, 0, 0, 34, 92, 64, 0, 50, 127, 0, 46, 0, 10, 0, 114, 17 } },   // A56
    { "Baixo sub", "Só o sub e o filtro pechado: case unha sinusoide.",
      { 0, 0, 0, 0, 0, 40, 0, 0, 0, 40, 15, 0, 0, 127, 10, 127, 98, 25 } },   // A57
    { "Baixo suave", "Cadrada en 16', moi arredondada.",
      { 0, 0, 0, 0, 0, 38, 0, 14, 0, 40, 13, 3, 70, 80, 26, 0, 105, 25 } },   // A58
    { "Solista de serra", "Serra e sub, co vibrato atrasado.",
      { 72, 52, 26, 0, 0, 72, 28, 30, 0, 80, 88, 2, 60, 100, 20, 40, 114, 17 } },   // A61
    { "Solista cadrado", "Cadrada e vibrato: o solista oco.",
      { 70, 50, 22, 0, 0, 66, 14, 18, 0, 80, 46, 2, 60, 110, 20, 0, 106, 17 } },   // A62
    { "Solista fino", "Pulso moi estreito e o paso alto: nasal.",
      { 70, 48, 18, 90, 0, 76, 20, 20, 0, 80, 77, 0, 0, 120, 16, 0, 106, 9 } },   // A63
    { "Frauta", "Cadrada en 4' cun pouco de aire.",
      { 68, 40, 16, 0, 14, 52, 0, 18, 0, 90, 49, 10, 60, 96, 20, 0, 108, 17 } },   // A64
    { "Asubío", "Sen osciladores: o filtro oscilando, afinado co teclado.",
      { 70, 30, 18, 0, 4, 62, 118, 0, 0, 127, 104, 10, 60, 110, 24, 0, 98, 17 } },   // A65
    { "Solista resonante", "Serra e un filtro que asubía co ataque.",
      { 70, 60, 16, 0, 0, 40, 84, 50, 0, 80, 115, 0, 64, 60, 20, 0, 114, 17 } },   // A66
    { "Solista brillante", "Serra e pulso en 4' co chorus II.",
      { 0, 0, 0, 24, 0, 80, 16, 20, 0, 60, 37, 0, 0, 110, 24, 0, 28, 9 } },   // A67
    { "Solista suave", "Cadrada pechada e un ataque lento.",
      { 66, 60, 16, 0, 0, 48, 0, 14, 0, 80, 55, 16, 60, 110, 30, 0, 74, 17 } },   // A68
    { "Fondo morno", "Serra e PWM con ataque e caída lentos: o fondo cálido do Juno.",
      { 24, 0, 0, 50, 0, 58, 6, 10, 0, 40, 29, 50, 82, 100, 80, 0, 26, 16 } },   // A71
    { "Fondo PWM", "Só o pulso, movéndose amodo.",
      { 22, 0, 0, 70, 0, 72, 0, 0, 0, 40, 39, 40, 80, 110, 78, 0, 74, 16 } },   // A72
    { "Coro", "Serra, PWM e un pouco de alento: voces sen palabras.",
      { 30, 0, 0, 56, 6, 52, 30, 0, 0, 40, 58, 56, 80, 110, 74, 0, 26, 16 } },   // A73
    { "Varrido", "O filtro ábrese moi amodo e volve pechar.",
      { 20, 0, 0, 40, 0, 30, 46, 50, 0, 40, 48, 100, 104, 40, 90, 0, 90, 16 } },   // A74
    { "Cristal", "Cadrada e serra en 4' co paso alto: frío e brillante.",
      { 0, 0, 0, 0, 0, 88, 0, 8, 0, 40, 43, 40, 80, 100, 100, 0, 28, 1 } },   // A75
    { "Fondo escuro", "En 16' e pechado, para debaixo de todo.",
      { 0, 0, 0, 0, 0, 36, 10, 8, 0, 40, 80, 70, 80, 110, 90, 60, 81, 17 } },   // A76
    { "Alento", "Ruído e un pouco de serra a través dun filtro resonante.",
      { 0, 0, 0, 0, 70, 50, 46, 10, 0, 80, 98, 60, 80, 100, 80, 0, 18, 17 } },   // A77
    { "Espazo", "O LFO move o filtro moi amodo; ataque e relaxación longos.",
      { 14, 0, 0, 46, 0, 56, 30, 0, 26, 40, 47, 90, 100, 110, 110, 0, 26, 16 } },   // A78
    { "Vento", "Ruído e un filtro resonante que se move só.",
      { 14, 0, 0, 0, 127, 62, 74, 0, 52, 0, 103, 80, 100, 110, 100, 0, 2, 17 } },   // A81
    { "Ondas", "Ruído que vai e vén, coma o mar.",
      { 4, 0, 0, 0, 127, 60, 24, 0, 64, 0, 127, 90, 110, 110, 110, 0, 2, 17 } },   // A82
    { "Láser", "A envolvente ao máximo cara abaixo: piu.",
      { 0, 0, 0, 0, 0, 30, 104, 127, 0, 127, 127, 0, 34, 0, 24, 0, 114, 17 } },   // A83
    { "Campá", "Cadrada e un filtro resonante sen sustain.",
      { 0, 0, 0, 0, 0, 70, 64, 18, 0, 90, 100, 0, 90, 0, 90, 0, 10, 9 } },   // A84
    { "Bombo", "O filtro oscilando e a envolvente que cae: un golpe grave.",
      { 0, 0, 0, 0, 0, 10, 127, 56, 0, 0, 116, 0, 28, 0, 20, 0, 98, 17 } },   // A85
    { "Tom", "Ruído e o filtro oscilando, afinado co teclado.",
      { 0, 0, 0, 0, 60, 36, 110, 44, 0, 80, 127, 0, 40, 0, 34, 0, 98, 17 } },   // A86
    { "Caixa", "Ruído co filtro aberto e unha caída curta.",
      { 0, 0, 0, 0, 127, 84, 0, 30, 0, 0, 69, 0, 26, 0, 24, 0, 98, 9 } },   // A87
    { "Disparo", "Serra e unha envolvente rápida que baixa: zap.",
      { 0, 0, 0, 0, 0, 20, 60, 100, 0, 60, 127, 0, 22, 0, 20, 0, 114, 17 } },   // A88
    { "Acorde house", "Serra e pulso, curto e co chorus II: o acorde das pistas.",
      { 0, 0, 0, 24, 0, 56, 20, 44, 0, 60, 40, 0, 50, 0, 26, 0, 26, 17 } },   // B11
    { "Acorde profundo", "Escuro e cun pouco de cola, para deep house.",
      { 0, 0, 0, 0, 0, 48, 14, 24, 0, 60, 105, 0, 58, 20, 44, 0, 82, 9 } },   // B12
    { "Rave", "Serra e PWM rápido en 4': brillante e sucio.",
      { 60, 0, 0, 60, 0, 90, 30, 0, 0, 0, 37, 0, 0, 120, 20, 0, 28, 16 } },   // B13
    { "Baixo house", "Cadrada e sub, a envolvente pecha axiña.",
      { 0, 0, 0, 0, 0, 40, 36, 46, 0, 50, 89, 0, 32, 0, 10, 110, 106, 17 } },   // B14
    { "Piano de baile", "Piano brillante para acordes en contratempo.",
      { 0, 0, 0, 16, 0, 52, 6, 40, 0, 90, 46, 0, 70, 0, 34, 0, 90, 9 } },   // B15
    { "Golpe de metais", "Metais curtos e directos.",
      { 0, 0, 0, 0, 0, 46, 10, 50, 0, 60, 83, 3, 40, 30, 20, 0, 82, 17 } },   // B16
    { "Punteo house", "Pulso e serra con resonancia: un pluck para arpexos.",
      { 0, 0, 0, 40, 0, 42, 46, 54, 0, 70, 65, 0, 36, 0, 30, 0, 26, 17 } },   // B17
    { "Sirena de baile", "O DCO sobe e baixa co LFO: para os momentos de euforia.",
      { 40, 0, 110, 0, 0, 80, 30, 0, 0, 0, 96, 10, 0, 127, 40, 0, 18, 17 } },   // B18
    { "Fondo 1984", "Serra e PWM con chorus I: o fondo do synthpop.",
      { 26, 0, 0, 50, 0, 62, 8, 12, 0, 40, 26, 30, 80, 100, 60, 0, 90, 16 } },   // B21
    { "Arpexo", "Curto e con resonancia, feito para o secuenciador.",
      { 0, 0, 0, 0, 0, 38, 50, 58, 0, 70, 122, 0, 30, 0, 24, 0, 82, 17 } },   // B22
    { "Solista synthpop", "Serra e cadrada, vibrato atrasado e chorus.",
      { 70, 60, 20, 0, 0, 68, 20, 24, 0, 70, 37, 0, 0, 110, 24, 0, 90, 17 } },   // B23
    { "Baixo synthpop", "En 16', serra e sub, cun golpe do filtro.",
      { 0, 0, 0, 0, 0, 40, 30, 44, 0, 40, 117, 0, 40, 30, 12, 60, 113, 17 } },   // B24
    { "Metais synthpop", "Serra co chorus II, ataque rápido.",
      { 0, 0, 0, 0, 0, 50, 14, 46, 0, 60, 77, 4, 60, 60, 30, 0, 18, 17 } },   // B25
    { "Campás pop", "Cadrada en 4' e un filtro resonante: brillo de campás.",
      { 0, 0, 0, 0, 0, 60, 44, 24, 0, 80, 73, 0, 70, 0, 62, 0, 12, 17 } },   // B26
    { "Cordas pop", "Cordas abertas co paso alto, para non tapar a voz.",
      { 30, 0, 0, 44, 0, 76, 0, 8, 0, 40, 30, 24, 80, 110, 56, 0, 26, 8 } },   // B27
    { "Robot", "Pulso estreito, ruído e resonancia: unha voz de máquina.",
      { 0, 0, 0, 96, 12, 58, 70, 20, 0, 80, 107, 0, 0, 110, 16, 0, 106, 9 } },   // B28
    { "Teclas lo-fi", "Cadrada e sub cun vibrato rápido e pequeno: a cinta que treme.",
      { 92, 0, 12, 0, 0, 44, 0, 20, 0, 80, 62, 0, 84, 30, 40, 40, 74, 17 } },   // B31
    { "Piano gastado", "Serra pechada e un vibrato lento: piano de casete.",
      { 18, 0, 14, 0, 0, 46, 0, 34, 0, 90, 70, 0, 70, 0, 50, 0, 18, 17 } },   // B32
    { "Órgano de casete", "Cadrada e sub en porta, cun bambeo lento.",
      { 14, 0, 10, 0, 0, 58, 0, 0, 0, 0, 55, 0, 0, 127, 6, 70, 74, 21 } },   // B33
    { "Fondo nostálxico", "Ataque lento, chorus II e o ton que baila un pouco.",
      { 18, 0, 8, 40, 0, 54, 0, 0, 0, 40, 55, 60, 80, 110, 80, 0, 26, 16 } },   // B34
    { "Baixo morno", "Cadrada en 16' pechada: un baixo que non molesta.",
      { 0, 0, 0, 0, 0, 34, 0, 10, 0, 40, 18, 2, 60, 90, 20, 0, 105, 25 } },   // B35
    { "Guitarra de xoguete", "Pulso estreito, curto e co paso alto.",
      { 0, 0, 0, 86, 0, 60, 20, 30, 0, 80, 102, 0, 42, 0, 30, 0, 106, 1 } },   // B36
    { "Coro lo-fi", "Ruído e serra a través dun filtro escuro.",
      { 20, 0, 6, 0, 40, 46, 36, 0, 0, 60, 127, 50, 80, 110, 70, 0, 18, 17 } },   // B37
    { "Caixa de música", "Cadrada en 4', curta e co paso alto.",
      { 0, 0, 0, 0, 0, 62, 0, 14, 0, 80, 79, 0, 60, 0, 60, 0, 76, 1 } },   // B38
    { "Néboa", "Serra e ruído que aparecen amodo; o filtro respira.",
      { 8, 0, 0, 0, 40, 56, 20, 0, 16, 40, 100, 100, 100, 110, 110, 0, 18, 17 } },   // B41
    { "Mar de Vigo", "Ruído e ondas lentas no filtro.",
      { 6, 0, 0, 0, 127, 58, 30, 0, 42, 0, 127, 90, 110, 110, 120, 0, 2, 17 } },   // B42
    { "Faro", "Pulso que xira amodo, coma a luz dun faro.",
      { 12, 0, 0, 80, 0, 66, 0, 0, 0, 40, 44, 60, 90, 110, 96, 0, 74, 16 } },   // B43
    { "Choiva", "Ruído curto co paso alto: pingas.",
      { 0, 0, 0, 0, 127, 96, 20, 20, 0, 0, 89, 0, 18, 0, 18, 0, 98, 1 } },   // B44
    { "Ría", "Serra e un filtro que sobe e baixa coma a marea.",
      { 4, 0, 0, 30, 0, 40, 40, 0, 40, 40, 67, 40, 90, 110, 100, 0, 26, 17 } },   // B45
    { "Lume", "Ruído, resonancia e un pouco de LFO: crepita.",
      { 110, 0, 0, 0, 110, 56, 80, 0, 30, 0, 127, 20, 80, 90, 80, 0, 66, 17 } },   // B46
    { "Noite", "Escuro, en 16', co sub e moito chorus.",
      { 10, 0, 0, 50, 0, 34, 16, 0, 0, 40, 89, 80, 100, 110, 110, 50, 25, 16 } },   // B47
    { "Amencer", "Ábrese moi amodo ata quedar brillante.",
      { 20, 0, 0, 40, 0, 40, 0, 40, 0, 40, 58, 118, 110, 110, 100, 0, 26, 16 } },   // B48
    { "Baixo Barullo", "Serra, pulso e sub: groso e con golpe.",
      { 0, 0, 0, 30, 0, 36, 24, 46, 0, 40, 5, 0, 50, 40, 12, 110, 122, 25 } },   // B51
    { "Baixo seco", "Serra sen sustain: nota e silencio.",
      { 0, 0, 0, 0, 0, 30, 16, 56, 0, 40, 127, 0, 34, 0, 8, 0, 114, 17 } },   // B52
    { "Baixo resonante", "A resonancia canta en cada nota.",
      { 0, 0, 0, 0, 0, 32, 74, 50, 0, 50, 127, 0, 44, 20, 12, 0, 114, 17 } },   // B53
    { "Baixo PWM", "Pulso co PWM do LFO e sub.",
      { 30, 0, 0, 70, 0, 38, 10, 30, 0, 40, 16, 0, 60, 70, 14, 90, 106, 24 } },   // B54
    { "Baixo de goma", "Cadrada e un filtro que rebota.",
      { 0, 0, 0, 0, 0, 28, 60, 44, 0, 50, 121, 0, 46, 20, 14, 0, 106, 17 } },   // B55
    { "Baixo profundo", "Só sub e o realce de graves: para sentilo máis ca oílo.",
      { 0, 0, 0, 0, 0, 28, 0, 0, 0, 20, 53, 0, 0, 127, 12, 127, 98, 25 } },   // B56
    { "Baixo 16 pés", "Serra en 16': o baixo máis grave do Juno.",
      { 0, 0, 0, 0, 0, 44, 10, 30, 0, 40, 57, 0, 60, 60, 12, 0, 113, 25 } },   // B57
    { "Baixo deslizante", "Para tocar co portamento e POLY 2.",
      { 0, 0, 0, 0, 0, 46, 30, 20, 0, 60, 118, 0, 0, 110, 14, 60, 114, 17 }, true, 90, 1 },   // B58
    { "Solista Barullo", "Serra e sub, resonancia e vibrato: o solista da casa.",
      { 72, 56, 22, 0, 0, 60, 40, 34, 0, 80, 96, 0, 60, 90, 24, 50, 82, 17 } },   // B61
    { "Sinal de radio", "Pulso estreito que se move rápido, co paso alto.",
      { 70, 0, 0, 60, 0, 74, 26, 0, 0, 80, 110, 0, 0, 120, 16, 0, 106, 0 } },   // B62
    { "Deslizante", "Cadrada e serra para tocar co portamento.",
      { 70, 60, 16, 0, 0, 64, 24, 20, 0, 80, 28, 0, 0, 120, 20, 0, 122, 17 }, true, 90, 1 },   // B63
    { "Asubío do faro", "O filtro oscilando, cun vibrato lento.",
      { 30, 0, 30, 0, 0, 60, 120, 0, 0, 127, 98, 20, 0, 110, 40, 0, 66, 17 } },   // B64
    { "Solista de pulso", "Pulso medio, ataque inmediato.",
      { 70, 50, 18, 40, 0, 70, 16, 16, 0, 80, 50, 0, 0, 120, 18, 0, 106, 17 } },   // B65
    { "Óboe", "Pulso estreito e pechado, nasal.",
      { 74, 50, 14, 80, 0, 56, 30, 14, 0, 90, 72, 6, 60, 100, 18, 0, 106, 17 } },   // B66
    { "Clarinete", "Cadrada pechada en 8'.",
      { 70, 60, 12, 0, 0, 46, 10, 12, 0, 90, 56, 8, 60, 110, 18, 0, 106, 17 } },   // B67
    { "Gaita", "Pulso e serra con resonancia, ataque seco: unha gaita de fol.",
      { 0, 0, 0, 66, 0, 74, 24, 6, 0, 60, 27, 2, 0, 127, 10, 0, 122, 17 } },   // B68
    { "Piano Xuno", "O piano da casa: serra e pulso con chorus I.",
      { 0, 0, 0, 26, 0, 44, 8, 46, 0, 100, 37, 0, 78, 0, 48, 0, 90, 17 } },   // B71
    { "Eléctrico morno", "Cadrada e sub co vibrato lento.",
      { 40, 0, 10, 0, 0, 46, 0, 24, 0, 90, 48, 0, 84, 24, 50, 50, 10, 17 } },   // B72
    { "Clavi", "Pulso estreito, curto, con paso alto.",
      { 0, 0, 0, 90, 0, 58, 40, 36, 0, 90, 116, 0, 40, 10, 8, 0, 106, 1 } },   // B73
    { "Harpa", "Serra punteada co chorus II.",
      { 0, 0, 0, 0, 0, 48, 10, 40, 0, 90, 90, 0, 66, 0, 60, 0, 18, 9 } },   // B74
    { "Tambor de aceiro", "Cadrada e resonancia, curto.",
      { 0, 0, 0, 0, 0, 50, 70, 30, 0, 90, 92, 0, 50, 0, 40, 0, 74, 17 } },   // B75
    { "Kalimba", "Cadrada en 4', moi curta.",
      { 0, 0, 0, 0, 0, 50, 40, 30, 0, 100, 76, 0, 40, 0, 34, 0, 108, 17 } },   // B76
    { "Xilófono", "Serra en 4' sen sustain.",
      { 0, 0, 0, 0, 0, 54, 20, 34, 0, 100, 92, 0, 36, 0, 30, 0, 116, 17 } },   // B77
    { "Campás de igrexa", "Cadrada en 16' con resonancia e unha cola longa.",
      { 0, 0, 0, 0, 0, 60, 76, 26, 0, 80, 123, 0, 100, 0, 100, 0, 9, 9 } },   // B78
    { "Sirena", "O LFO no DCO ao máximo e lento.",
      { 20, 0, 127, 0, 0, 80, 0, 0, 0, 0, 30, 0, 0, 127, 30, 0, 106, 17 } },   // B81
    { "Motor", "En 16', o LFO rápido no filtro.",
      { 100, 0, 0, 0, 0, 40, 50, 0, 60, 0, 74, 10, 0, 127, 40, 80, 113, 17 } },   // B82
    { "Ovni", "Asubío do filtro co LFO rápido no ton.",
      { 96, 0, 90, 0, 0, 80, 118, 0, 0, 127, 112, 30, 0, 110, 80, 0, 2, 17 } },   // B83
    { "Teléfono", "Cadrada en 4' en porta, co paso alto.",
      { 0, 0, 0, 0, 0, 80, 0, 0, 0, 0, 69, 0, 0, 127, 2, 0, 108, 5 } },   // B84
    { "Tren", "Ruído co LFO no filtro.",
      { 56, 0, 0, 0, 127, 64, 40, 0, 70, 0, 118, 20, 0, 110, 60, 0, 98, 17 } },   // B85
    { "Gaivota", "Resonancia, envolvente e un vibrato que se arrastra.",
      { 70, 40, 60, 0, 0, 56, 100, 40, 0, 80, 122, 10, 60, 40, 40, 0, 18, 17 } },   // B86
    { "Trono", "Ruído moi grave cunha cola longa.",
      { 0, 0, 0, 0, 127, 18, 30, 40, 0, 0, 99, 0, 110, 0, 110, 0, 98, 25 } },   // B87
    { "Inicial", "O son de partida: serra, filtro aberto, sen nada máis.",
      { 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 54, 0, 0, 127, 10, 0, 114, 17 } },   // B88
};

} // namespace xuno::bank
