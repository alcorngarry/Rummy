@echo off
setlocal

if not exist build mkdir build
pushd build

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

echo Building game.dll...
  cl %CommonCompilerFlags% /DBUILD_DLL ..\Game\game.cpp -LD ^
  /link %CommonLinkerFlags% ^
  UserInterface.lib ^
  /EXPORT:game_init ^
  /EXPORT:game_update_and_render ^
  /EXPORT:game_update_input ^
  /PDB:Game_%random%.pdb ^
  /OUT:Game.dll

popd
