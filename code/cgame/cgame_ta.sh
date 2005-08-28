#!/bin/sh

mkdir -p vm
cd vm

CC="q3lcc -DQ3_VM  -DCGAME -DMISSIONPACK -S -Wf-target=bytecode -Wf-g -I../../cgame -I../../game -I../../ui"

$CC  ../cg_syscalls.c
$CC ../../game/bg_misc.c
$CC ../../game/bg_pmove.c
$CC ../../game/bg_slidemove.c
$CC ../../game/bg_lib.c
$CC ../../game/q_math.c
$CC ../../game/q_shared.c
$CC ../cg_consolecmds.c
$CC ../cg_draw.c
$CC ../cg_drawtools.c
$CC ../cg_effects.c
$CC ../cg_ents.c
$CC ../cg_event.c
$CC ../cg_info.c
$CC ../cg_localents.c
$CC ../cg_main.c
$CC ../cg_marks.c
$CC ../cg_players.c
$CC ../cg_playerstate.c
$CC ../cg_predict.c
$CC ../cg_scoreboard.c
$CC ../cg_servercmds.c
$CC ../cg_snapshot.c
$CC ../cg_view.c
$CC ../cg_weapons.c
$CC ../../ui/ui_shared.c
$CC ../cg_newdraw.c

q3asm -f ../cgame_ta

cd ..
