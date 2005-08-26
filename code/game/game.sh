#!/bin/sh

mkdir -p vm
cd vm

CC="q3lcc -DQ3_VM -S -Wf-target=bytecode -Wf-g -I../../cgame -I../../game -I../../q3_ui"

$CC  ../g_main.c
$CC  ../g_syscalls.c

$CC  ../bg_misc.c
$CC  ../bg_lib.c
$CC  ../bg_pmove.c
$CC  ../bg_slidemove.c
$CC  ../q_math.c
$CC  ../q_shared.c

$CC  ../ai_vcmd.c
$CC  ../ai_dmnet.c
$CC  ../ai_dmq3.c
$CC  ../ai_main.c
$CC  ../ai_chat.c
$CC  ../ai_cmd.c
$CC  ../ai_team.c

$CC  ../g_active.c
$CC  ../g_arenas.c
$CC  ../g_bot.c
$CC  ../g_client.c
$CC  ../g_cmds.c
$CC  ../g_combat.c
$CC  ../g_items.c
$CC  ../g_mem.c
$CC  ../g_misc.c
$CC  ../g_missile.c
$CC  ../g_mover.c
$CC  ../g_session.c
$CC  ../g_spawn.c
$CC  ../g_svcmds.c
$CC  ../g_target.c
$CC  ../g_team.c
$CC  ../g_trigger.c
$CC  ../g_utils.c
$CC  ../g_weapon.c

q3asm -f ../game

cd ..
