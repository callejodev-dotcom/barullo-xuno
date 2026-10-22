#!/bin/bash
# As probas de Barullo Xuno na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows, linux ou sanitizadores, coa compilación en build/
set -eo pipefail
SO="$1"
EXE=""; [ "$SO" = windows ] && EXE=".exe"
CONF=Release; [ "$SO" = sanitizadores ] && CONF=RelWithDebInfo
ET="build/BXEngineTest_artefacts/$CONF/BXEngineTest$EXE"
RT="build/BXRenderTest_artefacts/$CONF/BXRenderTest$EXE"

echo "::group::Motor (afinación, LFO, envolventes, VCF e velocidade)"
"$ET"
echo "::endgroup::"
echo "::group::Son (SysEx, programas, estado, .syx, asignación, bloques e os 128 sons)"
"$RT" proba.wav
echo "::endgroup::"
