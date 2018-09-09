@echo off

set CommonCompilerFlags=-MD -nologo -Gm- -GR- -EHa -Od -Oi -W4 -FC -Z7  
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib 
set DisabledExceptions= -wd4189 -wd4201 -wd4100 -wd4459 -wd4456 -wd4505
set InternalBuildVariables= -DSLOW_MODE=1 -DINTERNAL_MODE=1 -DWIN32_MODE=1 -DWIN32_TRANSPARENT_MODE=0
set PDB= -PDB:game_%random%.pdb

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
REM 64-bit build
del *.pdb > NUL 2> NUL

REM /Ox /fp:fast - optimization switches
cl %CommonCompilerFlags% -Fmgame.map  %InternalBuildVariables% %DisabledExceptions% ..\source\code\game.cpp -LD -link -incremental:no -opt:ref  %PDB% -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples
cl %CommonCompilerFlags% -Fmwin32_game.map %InternalBuildVariables% %DisabledExceptions% ..\source\code\win32_game.cpp -link %CommonLinkerFlags%
REM 32-bit build
REM cl %CommonCompilerFlags% %InternalBuildVariables% %DisabledExceptions% ..\source\code\win32_game.cpp \link -subsystem:windows,5.1 %CommonLinkerFlags%

popd
