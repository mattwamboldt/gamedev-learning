@echo off

set buildDir=%~dp0..\build
set tempDir=%~dp0..\temp
set sourceDir=%~dp0..\code

if not exist %buildDir% mkdir %buildDir%
if not exist %tempDir% mkdir %tempDir%

echo "Building Resource File"
rc /nologo /fo %tempDir%\app.res %sourceDir%\win32\resources\app.rc

:: ================
:: COMMON FLAGS
:: ================
:: -FC - Output errors with full paths
:: -nologo - supresses the startup banner
:: -WX - warnings as errors
:: -W4 - warning level (wall will implode with windows.h)
:: -wd{number} - disables a specific warning
:: -Gm- disables minimal rebuild
:: -GR- disables RTTI
:: -EHa- uses only structured exception handling
:: -MT - Embeds the crt into the obj instead of relying on an external crt (no need for vc redistributables)
set CompilerFlags=-MT -Gm- -GR- -EHa- -WX -W4 -wd4201 -wd4100 -Fo%tempDir%\ -nologo -FC -Z7 -DINTERNAL=1 -D_CRT_SECURE_NO_WARNINGS=1

set LinkerFlags=-incremental:no -opt:ref 
set Libs=winmm.lib user32.lib opengl32.lib gdi32.lib

pushd %buildDir%

echo "Building Game DLL"
del *.pdb > NUL 2> NUL
set hour=%time:~0,2%

if "%hour:~0,1%" == " "  set hour=0%hour:~1,1%
cl %CompilerFlags% -Fegame-core %sourceDir%\game\game.cpp /LD /link %LinkerFlags% -PDB:game-core_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%hour%%time:~3,2%%time:~6,2%.pdb

echo "Building Executable"
cl %CompilerFlags% -Fegame %sourceDir%\win32\main.cpp /link %LinkerFlags% %tempDir%\app.res %Libs%

popd