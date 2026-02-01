@echo off
setlocal

%1 rem this should be the platform
%2 rem this should be debug/release
%3 rem this should be the run option

if not exist build\shaders (
    mkdir build\shaders
)

robocopy InfiniteErl\shaders build\shaders /E /IS /IT

if not exist build\res (
    mkdir build\res
)

robocopy InfiniteErl\res build\res /E /IS /IT


if not exist build mkdir build
pushd build

rem --------------------------
rem Compiler flags
rem --------------------------
set CommonCompilerFlags=/Od /nologo /FC /w ^
    /MDd /Zi /Oi /GR- /Gm- /EHsc ^
    /DINTERNAL_BUILD=1 /DDEBUG_BUILD=1 ^
    /I"..\UserInterface" ^
    /I"..\InfiniteErl" ^
    /I"C:\Dev\third-party-dependencies\include-4.6" 

set CommonLinkerFlags= ^
/LIBPATH:"C:\Dev\third-party-dependencies\lib" ^
opengl32.lib ^
glfw3.lib ^
freetype.lib ^
user32.lib gdi32.lib shell32.lib winmm.lib ^
kernel32.lib ole32.lib oleaut32.lib uuid.lib ^
comdlg32.lib advapi32.lib ^
/incremental:no ^
/IGNORE:4042 /IGNORE:4099

del *.obj *.lib *.pdb *.exe > NUL 2> NUL

rem --------------------------
rem Build UserInterface
rem --------------------------
echo Building UserInterface.lib...
cl %CommonCompilerFlags% -c ..\UserInterface\user_interface.cpp
lib /OUT:UserInterface.lib user_interface.obj

rem --------------------------
rem Build Engine EXE
rem --------------------------
echo Building InfiniteErl.exe...
cl %CommonCompilerFlags% /DCONSOLE ^
   ..\InfiniteErl\*.cpp ^
   C:\Dev\third-party-dependencies\include\glad\glad.c ^
   /link %CommonLinkerFlags% ^
   UserInterface.lib ^
   /OUT:InfiniteErl.exe

rem --------------------------
rem Build Game DLL
rem --------------------------
    echo Building game.dll...
    cl %CommonCompilerFlags% /DBUILD_DLL ..\Game\game.cpp -LD ^
    /link %CommonLinkerFlags% ^
    UserInterface.lib ^
    /EXPORT:game_init ^
    /EXPORT:game_update_and_render ^
    /EXPORT:game_update_input ^
    /PDB:Game_%random%.pdb ^
    /OUT:Game.dll

echo Build complete.
InfiniteErl.exe
popd
