#!/usr/bin/env bash
# Recapture les images 3D des croquis (Git Bash). Lance une seconde instance de l'editeur,
# l'instance deja ouverte peut rester ouverte. Images : Tools/GameplayConfigSite/img/
#   bash Tools/GameplayConfigSite/capture/Capturer.sh            # toutes
#   ORA_CAPTURE_ONLY=jump,wall bash .../Capturer.sh             # quelques-unes
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
SITE="$(dirname "$HERE")"
ORA="$(cd "$SITE/../.." && pwd)"
PROJECT="${ORA_PROJECT_FILE:-S:/Programmation/Jeu/UE5/Byakuya/ORA/ProjetUE5/MovementORA 5.8/MovementORA.uproject}"
EDITOR="${UE_EDITOR:-/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe}"
LOG="$(dirname "$PROJECT")/Saved/Logs/SketchCapture.log"

mkdir -p "$SITE/img"
export ORA_CAPTURE_OUT="$(cygpath -w "$SITE/img")"
rm -f "$LOG"
"$EDITOR" "$(cygpath -w "$PROJECT")" -ExecCmds="py $(cygpath -m "$HERE/capture_sketches.py")" \
  -unattended -nosound -windowed -ResX=1280 -ResY=720 -NoLiveCoding -log=SketchCapture.log >/dev/null 2>&1 &
PID=$!

# Attend la fin (ou une erreur Python), 10 min max.
for _ in $(seq 1 120); do
  sleep 5
  grep -qaE "\[SketchCapture\] termine|LogPython: Error|Log file closed" "$LOG" 2>/dev/null && break
done
sleep 3
grep -aE "SketchCapture\]|LogPython: Error" "$LOG" | cut -c1-400
# En cas d'erreur l'editeur reste ouvert : on le ferme.
if grep -qa "LogPython: Error" "$LOG"; then kill "$PID" 2>/dev/null; taskkill //PID "$PID" //F >/dev/null 2>&1; exit 1; fi
ls -la "$SITE/img"
