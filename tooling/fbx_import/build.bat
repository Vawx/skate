@echo off

IF NOT DEFINED clset (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")

set exe_name="skate_fbx_import"
set build_options=

set debug_flags=/Zi /EHsc

set compile_flags=-nologo %debug_flags%

set lib_paths=/libpath:..\lib
set link_win32=User32.lib winmm.lib kernel32.lib gdi32.lib shell32.lib 
set link_zlib=zlib.lib
set link_flags=/link %link_win32% %link_zlib%

set include_dir=/I ../source/ /I ../include/ 
set core=../source/fbx_importer_entry.cpp ../source/fbx_importer.cpp ../source/fbx_zlib.cpp ../source/fbx_types.cpp ../include/ufbx/ufbx.c

if not exist build mkdir build
pushd build
cl %build_options% %compile_flags% %core% %include_dir% %link_flags% %lib_paths% /out:%exe_name%.exe
copy ..\data\* . >NUL
popd