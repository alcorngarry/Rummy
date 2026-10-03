@echo off
setlocal

rem ============================================================
rem Tac-Tile Release Build
rem ============================================================

rem --------------------------
rem Create Release Directories
rem --------------------------

if not exist release (
    mkdir release
)

if not exist release\shaders (
    mkdir release\shaders
)

if not exist release\res (
    mkdir release\res
)

rem --------------------------
rem Copy Shaders
rem --------------------------

echo Copying shaders...

robocopy InfiniteErl\shaders release\shaders /E /IS /IT > NUL

rem --------------------------
rem Copy Resources
rem --------------------------

echo Copying resources...

robocopy InfiniteErl\res release\res /E /IS /IT > NUL

rem --------------------------
rem Enter Release Directory
rem --------------------------

pushd release

rem --------------------------
rem Release Compiler Flags
rem --------------------------

set CommonCompilerFlags=/O2 /nologo /FC /w ^
    /MD /Oi /GR- /Gm- /EHsc ^
    /DNDEBUG ^
    /I"..\UserInterface" ^
    /I"..\InfiniteErl" ^
    /I"C:\Dev\third-party-dependencies\include-4.6"

rem --------------------------
rem Release Linker Flags
rem --------------------------

set CommonLinkerFlags= ^
    /LIBPATH:"C:\Dev\third-party-dependencies\lib" ^
    opengl32.lib ^
    glfw3.lib ^
    freetype.lib ^
    user32.lib gdi32.lib shell32.lib winmm.lib ^
    kernel32.lib ole32.lib oleaut32.lib uuid.lib ^
    comdlg32.lib advapi32.lib ^
    /incremental:no ^
    /OPT:REF ^
    /OPT:ICF ^
    /IGNORE:4042 /IGNORE:4099

rem --------------------------
rem Clean Previous Build Files
rem --------------------------

echo.
echo Cleaning previous release build...

del *.obj > NUL 2> NUL
del *.pdb > NUL 2> NUL
del *.exe > NUL 2> NUL

rem --------------------------
rem Build UserInterface
rem --------------------------

echo.
echo ==========================
echo Building UserInterface.lib
echo ==========================
echo.

cl %CommonCompilerFlags% ^
    -c ..\UserInterface\user_interface.cpp

if errorlevel 1 goto build_failed

lib /OUT:UserInterface.lib user_interface.obj

if errorlevel 1 goto build_failed

rem --------------------------
rem Build Engine EXE
rem --------------------------

echo.
echo ==========================
echo Building Rummy.exe
echo ==========================
echo.

cl %CommonCompilerFlags% ^
    ..\InfiniteErl\*.cpp ^
    C:\Dev\third-party-dependencies\include\glad\glad.c ^
    /link %CommonLinkerFlags% ^
    UserInterface.lib ^
    /OUT:Rummy.exe

if errorlevel 1 goto build_failed

rem --------------------------
rem Build Game DLL
rem --------------------------

echo.
echo ==========================
echo Building Game.dll
echo ==========================
echo.

cl %CommonCompilerFlags% /DBUILD_DLL ^
    ..\Game\game.cpp ^
    ..\Game\validations.cpp ^
    ..\Game\game_queue.cpp ^
    ..\Game\rounds.cpp ^
    ..\Game\profile.cpp ^
    ..\Game\erl_math.cpp ^
    -LD ^
    /link %CommonLinkerFlags% ^
    UserInterface.lib ^
    /EXPORT:game_init ^
    /EXPORT:game_update_and_render ^
    /EXPORT:game_update_input ^
    /OUT:Game.dll

if errorlevel 1 goto build_failed

rem --------------------------
rem Remove Build-Only Files
rem --------------------------

echo.
echo Cleaning build files...

del *.obj > NUL 2> NUL
del *.pdb > NUL 2> NUL

rem --------------------------
rem Build Complete
rem --------------------------

echo.
echo ==========================
echo RELEASE BUILD COMPLETE
echo ==========================
echo.
echo Release files are in:
echo %CD%
echo.
echo Contents:
echo   Rummy.exe
echo   Game.dll
echo   shaders\
echo   res\
echo.
echo ==========================
echo.

popd
exit /b 0

rem ============================================================
rem Build Failed
rem ============================================================

:build_failed

echo.
echo ==========================
echo RELEASE BUILD FAILED
echo ==========================
echo.
echo Check the compiler errors above.
echo.

popd
exit /b 1xit /b 1
