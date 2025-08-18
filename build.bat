@echo off

IF NOT DEFINED clset (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")

set exe_name="skate"
set build_options= /DDEBUG_LOG

set debug_flags=/Zi /EHsc
set compile_flags=-nologo %debug_flags%

set lib_paths=/libpath:..\lib
set link_win32=User32.lib winmm.lib kernel32.lib gdi32.lib shell32.lib 
set link_gl=opengl32.lib
set link_cglm=cglm.lib
set link_zlib=zlib.lib
set link_flags=/link %link_win32% %link_gl% %link_zlib%

set include_dir=/I ../source/ /I ../include/ 
set core=../source/skate_entry.cpp

if not exist build mkdir build
pushd build
cl %build_options% %compile_flags% %core% %include_dir% %link_flags% %lib_paths% /out:%exe_name%.exe
copy ..\data\* . >NUL
popd