rem make sure we have a safe environement
set LIBRARY=
set INCLUDE=

mkdir vm
cd vm

set cc=lcc -DMISSIONPACK -DQ3_VM -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1

%cc% ../ui_main.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_misc.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_lib.c
@if errorlevel 1 goto quit
%cc% ../../game/q_math.c
@if errorlevel 1 goto quit
%cc% ../../game/q_shared.c
@if errorlevel 1 goto quit
%cc% ../ui_atoms.c
@if errorlevel 1 goto quit
%cc% ../ui_players.c
@if errorlevel 1 goto quit
%cc% ../ui_util.c
@if errorlevel 1 goto quit
%cc% ../ui_shared.c
@if errorlevel 1 goto quit
%cc% ../ui_gameinfo.c
@if errorlevel 1 goto quit

q3asm -f ../ui
:quit
cd ..
