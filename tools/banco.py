#!/usr/bin/env python3
# Xera Source/Banco.h: os 128 sons propios de Barullo Xuno (A11–B88), deseñados de cero.
#   Grupo A: CLÁSICOS, as familias dun banco de fábrica dos 80.
#   Grupo B: ESCOITA, sons para baile, synthpop, lo-fi e as texturas da casa.
# Cada son son os 18 bytes do Juno-106 (os mesmos que van no SysEx), un nome e unha descrición.
#
# Uso: python3 tools/banco.py [tools/niveis.txt] > Source/Banco.h
#   niveis.txt (opcional): a sonoridade de cada son medida co banco SEN igualar
#   (python3 tools/banco.py > Source/Banco.h; BXRenderTest --niveis tools/niveis.txt);
#   cos niveis, o VCA LEVEL de cada son axústase para que todos soen parecido (−16 dB).
import sys

def P(name, desc, rate=0, delay=0, dlfo=0, pwm=0, noise=0, freq=127, res=0, env=0, vlfo=0, kybd=0,
      level=64, a=0, d=0, s=127, r=10, sub=0, rng=8, pulse=False, saw=True, chorus=0,
      pwmlfo=False, neg=False, gate=False, hpf=1):
    sw1 = {16: 1, 8: 2, 4: 4}[rng] | (8 if pulse else 0) | (16 if saw else 0)
    sw1 |= {0: 0x60, 1: 0x40, 2: 0x00}[chorus]          # apagado (lémbrase I), I, II
    sw2 = (0 if pwmlfo else 1) | (2 if neg else 0) | (4 if gate else 0) | ((3 - hpf) << 3)
    vals = [rate, delay, dlfo, pwm, noise, freq, res, env, vlfo, kybd, level, a, d, s, r, sub, sw1, sw2]
    for i, v in enumerate(vals):
        assert 0 <= v <= 127, (name, i, v)
    return (name, desc, vals)

A = [
    # ---- A1 metais
    P("Metais", "Metais de sintetizador: o filtro ábrese co ataque e pecha ata media altura.",
      freq=48, res=12, env=48, kybd=64, a=6, d=62, s=70, r=30, chorus=1, dlfo=14, delay=70, rate=68),
    P("Metais suaves", "Unha sección que entra amodo e incha, para acordes longos.",
      freq=44, res=8, env=40, kybd=50, a=42, d=80, s=90, r=45, chorus=1),
    P("Trompeta", "Serra e pulso, ataque rápido e un vibrato que chega tarde.",
      freq=54, res=18, env=42, kybd=80, a=5, d=55, s=64, r=22, pulse=True, pwm=30, rate=72, delay=64, dlfo=24),
    P("Trompas", "Escuras e redondas, en 8' co sub por debaixo.",
      freq=38, res=6, env=30, kybd=60, a=22, d=70, s=88, r=40, sub=34, chorus=1),
    P("Metais 80", "Brillantes e curtos, cun pouco de resonancia: o metal de sintetizador dos 80.",
      freq=50, res=28, env=58, kybd=70, a=3, d=58, s=48, r=34, chorus=2),
    P("Fanfarra", "Serra e pulso estreito xuntos, para riffs que corten.",
      freq=56, res=16, env=52, kybd=64, a=2, d=50, s=60, r=26, pulse=True, pwm=46),
    P("Sección", "Metais con PWM lento: a sección enteira, ancha.",
      freq=50, res=10, env=40, kybd=60, a=12, d=70, s=80, r=40, pulse=True, pwm=56, pwmlfo=True, rate=24, chorus=1),
    P("Metais e cordas", "Metade metal, metade corda: ataque lento e chorus.",
      freq=58, res=4, env=26, kybd=50, a=30, d=80, s=100, r=60, pulse=True, pwm=60, pwmlfo=True, rate=30, chorus=1),
    # ---- A2 cordas
    P("Cordas", "Conxunto de cordas co chorus I: o son polo que se coñece este sintetizador.",
      freq=72, res=0, env=12, kybd=40, a=40, d=70, s=110, r=62, chorus=1, dlfo=10, delay=60, rate=60, hpf=2),
    P("Cordas lentas", "Entran e saen amodo, para fondos.",
      freq=66, env=8, kybd=40, a=70, d=90, s=118, r=86, chorus=2, hpf=2),
    P("Conxunto", "Serra e PWM: moitas cordas, moi anchas.",
      freq=70, env=10, kybd=40, a=38, d=80, s=112, r=64, pulse=True, pwm=50, pwmlfo=True, rate=28, chorus=2, hpf=2),
    P("Violíns", "Brillantes e co vibrato atrasado, en 4'.",
      rng=4, freq=80, env=10, kybd=50, a=30, d=70, s=110, r=50, dlfo=22, delay=56, rate=74, chorus=1, hpf=2),
    P("Violonchelos", "En 16': corpo e arco.",
      rng=16, freq=52, res=6, env=20, kybd=40, a=28, d=70, s=100, r=50, dlfo=14, delay=60, rate=66, chorus=1),
    P("Pizzicato", "Cordas punteadas: sen sustain.",
      freq=50, res=10, env=44, kybd=80, a=0, d=36, s=0, r=32, chorus=1),
    P("Trémolo", "Cordas co LFO no filtro: un trémolo rápido.",
      freq=64, env=8, vlfo=34, rate=86, kybd=40, a=26, d=70, s=110, r=50, chorus=1, hpf=2),
    P("Cordas brillantes", "Abertas e cun pouco de pulso, para que se oian na mestura.",
      freq=88, env=6, kybd=30, a=34, d=70, s=110, r=58, pulse=True, pwm=40, pwmlfo=True, rate=34, chorus=2, hpf=2),
    # ---- A3 teclas
    P("Piano", "O piano de sintetizador: serra e pulso, e a caída do filtro.",
      freq=40, res=6, env=50, kybd=100, a=0, d=74, s=0, r=46, pulse=True, pwm=20, chorus=1),
    P("Piano eléctrico", "Cadrada e sub, suave: un eléctrico morno.",
      saw=False, pulse=True, sub=40, freq=48, env=30, kybd=90, a=0, d=80, s=20, r=46, chorus=1, dlfo=6, rate=80),
    P("Eléctrico brillante", "Pulso estreito: o eléctrico con máis ataque.",
      saw=False, pulse=True, pwm=46, freq=52, res=14, env=40, kybd=90, a=0, d=76, s=16, r=44, chorus=2),
    P("Clavinete", "Pulso estreito, resonancia e o paso alto: funk.",
      saw=False, pulse=True, pwm=80, freq=56, res=34, env=40, kybd=90, a=0, d=44, s=14, r=10, hpf=2),
    P("Clave", "Clavecín en 4', moi aberto e sen sustain.",
      rng=4, pulse=True, pwm=30, freq=66, env=36, kybd=80, a=0, d=56, s=0, r=40, chorus=2, hpf=3),
    P("Celesta", "Cadrada en 4', doce.",
      rng=4, saw=False, pulse=True, freq=58, env=20, kybd=80, a=0, d=70, s=0, r=62, chorus=1),
    P("Vibráfono", "Cadrada co LFO no filtro: o trémolo do vibráfono.",
      saw=False, pulse=True, freq=52, env=16, vlfo=22, rate=80, kybd=90, a=0, d=82, s=0, r=72, chorus=1),
    P("Marimba", "Cadrada e sub, curta e de madeira.",
      saw=False, pulse=True, sub=50, freq=36, res=32, env=54, kybd=100, a=0, d=40, s=0, r=36),
    # ---- A4 órganos
    P("Órgano", "Cadrada e sub co VCA en porta: o órgano de sempre, co chorus II.",
      saw=False, pulse=True, sub=90, freq=100, gate=True, r=4, chorus=2, level=58),
    P("Órgano percutido", "Cun golpe do filtro ao empezar cada nota.",
      saw=False, pulse=True, sub=80, freq=66, env=40, kybd=60, a=0, d=34, s=80, r=6, chorus=2, level=58),
    P("Órgano combo", "Pulso e serra en porta: o órgano dos grupos dos 60.",
      pulse=True, pwm=60, freq=94, gate=True, r=4, chorus=1, level=56),
    P("Órgano de igrexa", "En 16' e con todo: serra, pulso e sub.",
      rng=16, pulse=True, sub=100, freq=88, a=16, s=127, r=70, chorus=2, level=54),
    P("Calíope", "Pulso estreito, un pouco de aire e vibrato.",
      saw=False, pulse=True, pwm=70, noise=10, freq=80, gate=True, r=6, dlfo=30, rate=76),
    P("Harmonio", "Serra con PWM lento, coma un fol.",
      pulse=True, pwm=40, pwmlfo=True, rate=20, freq=64, res=14, a=14, s=120, r=30, chorus=1),
    P("Acordeón", "Serra e pulso cun vibrato rápido e pequeno.",
      pulse=True, pwm=30, freq=74, res=8, a=6, s=120, r=16, dlfo=10, rate=96, chorus=2),
    P("Órgano house", "Cadrada e sub co filtro pechando axiña: o órgano das pistas de baile dos 90.",
      saw=False, pulse=True, sub=100, freq=44, res=40, env=56, kybd=60, a=0, d=50, s=0, r=30, chorus=0),
    # ---- A5 baixos
    P("Baixo", "Serra e sub, co realce de graves.",
      sub=100, freq=34, res=20, env=48, kybd=40, a=0, d=56, s=40, r=10, hpf=0),
    P("Baixo redondo", "Cadrada e todo o sub: un baixo cheo.",
      saw=False, pulse=True, sub=127, freq=30, res=10, env=30, kybd=40, a=0, d=60, s=60, r=12, hpf=0),
    P("Baixo punteado", "Serra moi curta e con resonancia.",
      freq=30, res=56, env=68, kybd=50, a=0, d=44, s=0, r=16),
    P("Baixo funk", "Pulso estreito, sub e un filtro que fala.",
      saw=False, pulse=True, pwm=60, sub=70, freq=30, res=50, env=64, kybd=40, a=0, d=36, s=10, r=12),
    P("Baixo groso", "Serra, PWM e sub co chorus I: o baixo groso de seis voces.",
      pulse=True, pwm=50, pwmlfo=True, rate=26, sub=90, freq=40, res=14, env=36, kybd=40, a=0, d=60, s=60, r=14, chorus=1, hpf=0),
    P("Baixo ácido", "Resonancia alta e a envolvente ao fondo.",
      freq=34, res=92, env=64, kybd=50, a=0, d=46, s=0, r=10, level=64),
    P("Baixo sub", "Só o sub e o filtro pechado: case unha sinusoide.",
      saw=False, pulse=False, sub=127, freq=40, kybd=40, a=0, d=0, s=127, r=10, hpf=0, level=72),
    P("Baixo suave", "Cadrada en 16', moi arredondada.",
      rng=16, saw=False, pulse=True, freq=38, env=14, kybd=40, a=3, d=70, s=80, r=26, hpf=0),
    # ---- A6 solistas
    P("Solista de serra", "Serra e sub, co vibrato atrasado.",
      sub=40, freq=72, res=28, env=30, kybd=80, a=2, d=60, s=100, r=20, dlfo=26, delay=52, rate=72),
    P("Solista cadrado", "Cadrada e vibrato: o solista oco.",
      saw=False, pulse=True, freq=66, res=14, env=18, kybd=80, a=2, d=60, s=110, r=20, dlfo=22, delay=50, rate=70),
    P("Solista fino", "Pulso moi estreito e o paso alto: nasal.",
      saw=False, pulse=True, pwm=90, freq=76, res=20, env=20, kybd=80, a=0, s=120, r=16, hpf=2, dlfo=18, delay=48, rate=70),
    P("Frauta", "Cadrada en 4' cun pouco de aire.",
      rng=4, saw=False, pulse=True, noise=14, freq=52, env=18, kybd=90, a=10, d=60, s=96, r=20, dlfo=16, delay=40, rate=68),
    P("Asubío", "Sen osciladores: o filtro oscilando, afinado co teclado.",
      saw=False, pulse=False, noise=4, freq=62, res=118, kybd=127, a=10, d=60, s=110, r=24, dlfo=18, delay=30, rate=70, level=74),
    P("Solista resonante", "Serra e un filtro que asubía co ataque.",
      freq=40, res=84, env=50, kybd=80, a=0, d=64, s=60, r=20, dlfo=16, delay=60, rate=70, level=60),
    P("Solista brillante", "Serra e pulso en 4' co chorus II.",
      rng=4, pulse=True, pwm=24, freq=80, res=16, env=20, kybd=60, a=0, s=110, r=24, chorus=2, hpf=2),
    P("Solista suave", "Cadrada pechada e un ataque lento.",
      saw=False, pulse=True, freq=48, env=14, kybd=80, a=16, d=60, s=110, r=30, dlfo=16, delay=60, rate=66, chorus=1),
    # ---- A7 fondos
    P("Fondo morno", "Serra e PWM con ataque e caída lentos: o fondo cálido do Juno.",
      pulse=True, pwm=50, pwmlfo=True, rate=24, freq=58, res=6, env=10, kybd=40, a=50, d=82, s=100, r=80, chorus=2),
    P("Fondo PWM", "Só o pulso, movéndose amodo.",
      saw=False, pulse=True, pwm=70, pwmlfo=True, rate=22, freq=72, kybd=40, a=40, d=80, s=110, r=78, chorus=1),
    P("Coro", "Serra, PWM e un pouco de alento: voces sen palabras.",
      pulse=True, pwm=56, pwmlfo=True, rate=30, noise=6, freq=52, res=30, kybd=40, a=56, d=80, s=110, r=74, chorus=2),
    P("Varrido", "O filtro ábrese moi amodo e volve pechar.",
      pulse=True, pwm=40, pwmlfo=True, rate=20, freq=30, res=46, env=50, kybd=40, a=100, d=104, s=40, r=90, chorus=1),
    P("Cristal", "Cadrada e serra en 4' co paso alto: frío e brillante.",
      rng=4, pulse=True, freq=88, env=8, kybd=40, a=40, d=80, s=100, r=100, chorus=2, hpf=3),
    P("Fondo escuro", "En 16' e pechado, para debaixo de todo.",
      rng=16, sub=60, freq=36, res=10, env=8, kybd=40, a=70, d=80, s=110, r=90, chorus=1),
    P("Alento", "Ruído e un pouco de serra a través dun filtro resonante.",
      noise=70, freq=50, res=46, env=10, kybd=80, a=60, d=80, s=100, r=80, chorus=2, level=66),
    P("Espazo", "O LFO move o filtro moi amodo; ataque e relaxación longos.",
      pulse=True, pwm=46, pwmlfo=True, rate=14, freq=56, res=30, vlfo=26, kybd=40, a=90, d=100, s=110, r=110, chorus=2),
    # ---- A8 efectos e percusión
    P("Vento", "Ruído e un filtro resonante que se move só.",
      saw=False, noise=127, freq=62, res=74, vlfo=52, rate=14, a=80, d=100, s=110, r=100, chorus=2, level=60),
    P("Ondas", "Ruído que vai e vén, coma o mar.",
      saw=False, noise=127, freq=60, res=24, vlfo=64, rate=4, a=90, d=110, s=110, r=110, chorus=2, level=62),
    P("Láser", "A envolvente ao máximo cara abaixo: piu.",
      freq=30, res=104, env=127, kybd=127, a=0, d=34, s=0, r=24, level=56),
    P("Campá", "Cadrada e un filtro resonante sen sustain.",
      saw=False, pulse=True, freq=70, res=64, env=18, kybd=90, a=0, d=90, s=0, r=90, chorus=2, hpf=2),
    P("Bombo", "O filtro oscilando e a envolvente que cae: un golpe grave.",
      saw=False, pulse=False, freq=10, res=127, env=56, kybd=0, a=0, d=28, s=0, r=20, gate=False, level=80),
    P("Tom", "Ruído e o filtro oscilando, afinado co teclado.",
      saw=False, noise=60, freq=36, res=110, env=44, kybd=80, a=0, d=40, s=0, r=34, level=70),
    P("Caixa", "Ruído co filtro aberto e unha caída curta.",
      saw=False, noise=127, freq=84, env=30, a=0, d=26, s=0, r=24, hpf=2, level=62),
    P("Disparo", "Serra e unha envolvente rápida que baixa: zap.",
      freq=20, res=60, env=100, kybd=60, a=0, d=22, s=0, r=20),
]

B = [
    # ---- B1 baile
    P("Acorde house", "Serra e pulso, curto e co chorus II: o acorde das pistas.",
      pulse=True, pwm=24, freq=56, res=20, env=44, kybd=60, a=0, d=50, s=0, r=26, chorus=2),
    P("Acorde profundo", "Escuro e cun pouco de cola, para deep house.",
      freq=48, res=14, env=24, kybd=60, a=0, d=58, s=20, r=44, chorus=1, hpf=2),
    P("Rave", "Serra e PWM rápido en 4': brillante e sucio.",
      rng=4, pulse=True, pwm=60, pwmlfo=True, rate=60, freq=90, res=30, a=0, s=120, r=20, chorus=2),
    P("Baixo house", "Cadrada e sub, a envolvente pecha axiña.",
      saw=False, pulse=True, sub=110, freq=40, res=36, env=46, kybd=50, a=0, d=32, s=0, r=10),
    P("Piano de baile", "Piano brillante para acordes en contratempo.",
      pulse=True, pwm=16, freq=52, res=6, env=40, kybd=90, a=0, d=70, s=0, r=34, chorus=1, hpf=2),
    P("Golpe de metais", "Metais curtos e directos.",
      freq=46, res=10, env=50, kybd=60, a=3, d=40, s=30, r=20, chorus=1),
    P("Punteo house", "Pulso e serra con resonancia: un pluck para arpexos.",
      pulse=True, pwm=40, freq=42, res=46, env=54, kybd=70, a=0, d=36, s=0, r=30, chorus=2),
    P("Sirena de baile", "O DCO sobe e baixa co LFO: para os momentos de euforia.",
      dlfo=110, rate=40, freq=80, res=30, a=10, s=127, r=40, chorus=2, level=58),
    # ---- B2 synthpop
    P("Fondo 1984", "Serra e PWM con chorus I: o fondo do synthpop.",
      pulse=True, pwm=50, pwmlfo=True, rate=26, freq=62, res=8, env=12, kybd=40, a=30, d=80, s=100, r=60, chorus=1),
    P("Arpexo", "Curto e con resonancia, feito para o secuenciador.",
      freq=38, res=50, env=58, kybd=70, a=0, d=30, s=0, r=24, chorus=1),
    P("Solista synthpop", "Serra e cadrada, vibrato atrasado e chorus.",
      pulse=True, freq=68, res=20, env=24, kybd=70, a=0, s=110, r=24, dlfo=20, delay=60, rate=70, chorus=1),
    P("Baixo synthpop", "En 16', serra e sub, cun golpe do filtro.",
      rng=16, sub=60, freq=40, res=30, env=44, kybd=40, a=0, d=40, s=30, r=12),
    P("Metais synthpop", "Serra co chorus II, ataque rápido.",
      freq=50, res=14, env=46, kybd=60, a=4, d=60, s=60, r=30, chorus=2),
    P("Campás pop", "Cadrada en 4' e un filtro resonante: brillo de campás.",
      rng=4, saw=False, pulse=True, freq=60, res=44, env=24, kybd=80, a=0, d=70, s=0, r=62, chorus=2),
    P("Cordas pop", "Cordas abertas co paso alto, para non tapar a voz.",
      pulse=True, pwm=44, pwmlfo=True, rate=30, freq=76, env=8, kybd=40, a=24, d=80, s=110, r=56, chorus=2, hpf=2),
    P("Robot", "Pulso estreito, ruído e resonancia: unha voz de máquina.",
      saw=False, pulse=True, pwm=96, noise=12, freq=58, res=70, env=20, kybd=80, a=0, s=110, r=16, hpf=2),
    # ---- B3 lo-fi
    P("Teclas lo-fi", "Cadrada e sub cun vibrato rápido e pequeno: a cinta que treme.",
      saw=False, pulse=True, sub=40, freq=44, env=20, kybd=80, a=0, d=84, s=30, r=40, dlfo=12, rate=92, chorus=1),
    P("Piano gastado", "Serra pechada e un vibrato lento: piano de casete.",
      freq=46, env=34, kybd=90, a=0, d=70, s=0, r=50, dlfo=14, rate=18, chorus=2),
    P("Órgano de casete", "Cadrada e sub en porta, cun bambeo lento.",
      saw=False, pulse=True, sub=70, freq=58, gate=True, r=6, dlfo=10, rate=14, chorus=1, level=58),
    P("Fondo nostálxico", "Ataque lento, chorus II e o ton que baila un pouco.",
      pulse=True, pwm=40, pwmlfo=True, rate=18, freq=54, kybd=40, a=60, d=80, s=110, r=80, dlfo=8, chorus=2),
    P("Baixo morno", "Cadrada en 16' pechada: un baixo que non molesta.",
      rng=16, saw=False, pulse=True, freq=34, env=10, kybd=40, a=2, d=60, s=90, r=20, hpf=0),
    P("Guitarra de xoguete", "Pulso estreito, curto e co paso alto.",
      saw=False, pulse=True, pwm=86, freq=60, res=20, env=30, kybd=80, a=0, d=42, s=0, r=30, hpf=3),
    P("Coro lo-fi", "Ruído e serra a través dun filtro escuro.",
      noise=40, freq=46, res=36, kybd=60, a=50, d=80, s=110, r=70, dlfo=6, rate=20, chorus=2),
    P("Caixa de música", "Cadrada en 4', curta e co paso alto.",
      rng=4, saw=False, pulse=True, freq=62, env=14, kybd=80, a=0, d=60, s=0, r=60, chorus=1, hpf=3),
    # ---- B4 texturas de Escoita
    P("Néboa", "Serra e ruído que aparecen amodo; o filtro respira.",
      noise=40, freq=56, res=20, vlfo=16, rate=8, kybd=40, a=100, d=100, s=110, r=110, chorus=2),
    P("Mar de Vigo", "Ruído e ondas lentas no filtro.",
      saw=False, noise=127, freq=58, res=30, vlfo=42, rate=6, a=90, d=110, s=110, r=120, chorus=2, level=62),
    P("Faro", "Pulso que xira amodo, coma a luz dun faro.",
      saw=False, pulse=True, pwm=80, pwmlfo=True, rate=12, freq=66, kybd=40, a=60, d=90, s=110, r=96, chorus=1),
    P("Choiva", "Ruído curto co paso alto: pingas.",
      saw=False, noise=127, freq=96, res=20, env=20, a=0, d=18, s=0, r=18, hpf=3, level=60),
    P("Ría", "Serra e un filtro que sobe e baixa coma a marea.",
      pulse=True, pwm=30, freq=40, res=40, vlfo=40, rate=4, kybd=40, a=40, d=90, s=110, r=100, chorus=2),
    P("Lume", "Ruído, resonancia e un pouco de LFO: crepita.",
      saw=False, noise=110, freq=56, res=80, vlfo=30, rate=110, a=20, d=80, s=90, r=80, chorus=1, level=58),
    P("Noite", "Escuro, en 16', co sub e moito chorus.",
      rng=16, pulse=True, pwm=50, pwmlfo=True, rate=10, sub=50, freq=34, res=16, kybd=40, a=80, d=100, s=110, r=110, chorus=2),
    P("Amencer", "Ábrese moi amodo ata quedar brillante.",
      pulse=True, pwm=40, pwmlfo=True, rate=20, freq=40, env=40, kybd=40, a=118, d=110, s=110, r=100, chorus=2),
    # ---- B5 baixos
    P("Baixo Barullo", "Serra, pulso e sub: groso e con golpe.",
      pulse=True, pwm=30, sub=110, freq=36, res=24, env=46, kybd=40, a=0, d=50, s=40, r=12, hpf=0),
    P("Baixo seco", "Serra sen sustain: nota e silencio.",
      freq=30, res=16, env=56, kybd=40, a=0, d=34, s=0, r=8),
    P("Baixo resonante", "A resonancia canta en cada nota.",
      freq=32, res=74, env=50, kybd=50, a=0, d=44, s=20, r=12, level=64),
    P("Baixo PWM", "Pulso co PWM do LFO e sub.",
      saw=False, pulse=True, pwm=70, pwmlfo=True, rate=30, sub=90, freq=38, res=10, env=30, kybd=40, a=0, d=60, s=70, r=14, hpf=0),
    P("Baixo de goma", "Cadrada e un filtro que rebota.",
      saw=False, pulse=True, freq=28, res=60, env=44, kybd=50, a=0, d=46, s=20, r=14),
    P("Baixo profundo", "Só sub e o realce de graves: para sentilo máis ca oílo.",
      saw=False, pulse=False, sub=127, freq=28, kybd=20, a=0, s=127, r=12, hpf=0, level=76),
    P("Baixo 16 pés", "Serra en 16': o baixo máis grave do Juno.",
      rng=16, freq=44, res=10, env=30, kybd=40, a=0, d=60, s=60, r=12, hpf=0),
    P("Baixo deslizante", "Para tocar co portamento e POLY 2.",
      sub=60, freq=46, res=30, env=20, kybd=60, a=0, s=110, r=14),
    # ---- B6 solistas
    P("Solista Barullo", "Serra e sub, resonancia e vibrato: o solista da casa.",
      sub=50, freq=60, res=40, env=34, kybd=80, a=0, d=60, s=90, r=24, dlfo=22, delay=56, rate=72, chorus=1),
    P("Sinal de radio", "Pulso estreito que se move rápido, co paso alto.",
      saw=False, pulse=True, pwm=60, pwmlfo=True, rate=70, freq=74, res=26, kybd=80, a=0, s=120, r=16, hpf=3),
    P("Deslizante", "Cadrada e serra para tocar co portamento.",
      pulse=True, freq=64, res=24, env=20, kybd=80, a=0, s=120, r=20, dlfo=16, delay=60, rate=70),
    P("Asubío do faro", "O filtro oscilando, cun vibrato lento.",
      saw=False, pulse=False, freq=60, res=120, kybd=127, a=20, s=110, r=40, dlfo=30, rate=30, chorus=1, level=74),
    P("Solista de pulso", "Pulso medio, ataque inmediato.",
      saw=False, pulse=True, pwm=40, freq=70, res=16, env=16, kybd=80, a=0, s=120, r=18, dlfo=18, delay=50, rate=70),
    P("Óboe", "Pulso estreito e pechado, nasal.",
      saw=False, pulse=True, pwm=80, freq=56, res=30, env=14, kybd=90, a=6, d=60, s=100, r=18, dlfo=14, delay=50, rate=74),
    P("Clarinete", "Cadrada pechada en 8'.",
      saw=False, pulse=True, freq=46, res=10, env=12, kybd=90, a=8, d=60, s=110, r=18, dlfo=12, delay=60, rate=70),
    P("Gaita", "Pulso e serra con resonancia, ataque seco: unha gaita de fol.",
      pulse=True, pwm=66, freq=74, res=24, env=6, kybd=60, a=2, s=127, r=10, level=58),
    # ---- B7 teclas
    P("Piano Xuno", "O piano da casa: serra e pulso con chorus I.",
      pulse=True, pwm=26, freq=44, res=8, env=46, kybd=100, a=0, d=78, s=0, r=48, chorus=1),
    P("Eléctrico morno", "Cadrada e sub co vibrato lento.",
      saw=False, pulse=True, sub=50, freq=46, env=24, kybd=90, a=0, d=84, s=24, r=50, dlfo=10, rate=40, chorus=2),
    P("Clavi", "Pulso estreito, curto, con paso alto.",
      saw=False, pulse=True, pwm=90, freq=58, res=40, env=36, kybd=90, a=0, d=40, s=10, r=8, hpf=3),
    P("Harpa", "Serra punteada co chorus II.",
      freq=48, res=10, env=40, kybd=90, a=0, d=66, s=0, r=60, chorus=2, hpf=2),
    P("Tambor de aceiro", "Cadrada e resonancia, curto.",
      saw=False, pulse=True, freq=50, res=70, env=30, kybd=90, a=0, d=50, s=0, r=40, chorus=1),
    P("Kalimba", "Cadrada en 4', moi curta.",
      rng=4, saw=False, pulse=True, freq=50, res=40, env=30, kybd=100, a=0, d=40, s=0, r=34),
    P("Xilófono", "Serra en 4' sen sustain.",
      rng=4, freq=54, res=20, env=34, kybd=100, a=0, d=36, s=0, r=30),
    P("Campás de igrexa", "Cadrada en 16' con resonancia e unha cola longa.",
      rng=16, saw=False, pulse=True, freq=60, res=76, env=26, kybd=80, a=0, d=100, s=0, r=100, chorus=2, hpf=2, level=60),
    # ---- B8 efectos
    P("Sirena", "O LFO no DCO ao máximo e lento.",
      saw=False, pulse=True, dlfo=127, rate=20, freq=80, a=0, s=127, r=30, level=56),
    P("Motor", "En 16', o LFO rápido no filtro.",
      rng=16, sub=80, freq=40, res=50, vlfo=60, rate=100, a=10, s=127, r=40),
    P("Ovni", "Asubío do filtro co LFO rápido no ton.",
      saw=False, pulse=False, freq=80, res=118, kybd=127, dlfo=90, rate=96, a=30, s=110, r=80, chorus=2, level=70),
    P("Teléfono", "Cadrada en 4' en porta, co paso alto.",
      rng=4, saw=False, pulse=True, freq=80, gate=True, r=2, hpf=3, level=56),
    P("Tren", "Ruído co LFO no filtro.",
      saw=False, noise=127, freq=64, res=40, vlfo=70, rate=56, a=20, s=110, r=60, level=60),
    P("Gaivota", "Resonancia, envolvente e un vibrato que se arrastra.",
      freq=56, res=100, env=40, kybd=80, dlfo=60, rate=70, delay=40, a=10, d=60, s=40, r=40, chorus=2, level=60),
    P("Trono", "Ruído moi grave cunha cola longa.",
      saw=False, noise=127, freq=18, res=30, env=40, a=0, d=110, s=0, r=110, hpf=0, level=72),
    P("Inicial", "O son de partida: serra, filtro aberto, sen nada máis.",
      freq=127, a=0, d=0, s=127, r=10),
]

assert len(A) == 64, len(A)
assert len(B) == 64, len(B)
allp = A + B

# niveis medidos (opcional): igualar a sonoridade co VCA LEVEL (0,157 dB por paso)
if len(sys.argv) > 1:
    meas = {}
    for line in open(sys.argv[1]):
        k, v = line.split()
        meas[int(k)] = float(v)
    target = -16.0
    out = []
    for i, (n, d, v) in enumerate(allp):
        if i in meas:
            v = list(v)
            v[10] = max(0, min(127, int(round(v[10] + (target - meas[i]) / 0.157))))
        out.append((n, d, v))
    allp = out

print('// Banco.h — Xerado por tools/banco.py. Non editar a man.')
print('// Os 128 sons propios de Barullo Xuno: A11–A88 clásicos, B11–B88 Escoita.')
print('#pragma once')
print('#include <cstdint>\n')
print('namespace xuno::bank {\n')
print('struct Entry { const char* name; const char* desc; uint8_t p[18]; };\n')
print('inline const Entry PATCHES[128] = {')
for i, (n, d, v) in enumerate(allp):
    lab = ('A' if i < 64 else 'B') + str((i % 64) // 8 + 1) + str(i % 8 + 1)
    print('    { "%s", "%s",' % (n, d))
    print('      { %s } },   // %s' % (', '.join(str(x) for x in v), lab))
print('};\n')
print('} // namespace xuno::bank')
