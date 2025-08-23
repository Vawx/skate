@echo off

IF NOT DEFINED clset (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")

set exe_name="skate"
set jolt_build_options=/DJPH_DEBUG_RENDERER /DWIN32 /D_WINDOWS /D_HAS_EXCEPTIONS=0 /DNDEBUG /DJPH_OBJECT_STREAM /DJPH_USE_AVX2 /DJPH_USE_AVX /DJPH_USE_SSE4_1 /DJPH_USE_SSE4_2 /DJPH_USE_LZCNT /DJPH_USE_TZCNT /DJPH_USE_F16C /DJPH_USE_FMADD 
set build_options= /DDEBUG_LOG %jolt_build_options% 

set debug_flags=/Od /Zi /EHsc
set compile_flags=-nologo %debug_flags% /std:c++17 /Zc:__cplusplus

set lib_paths=/libpath:..\lib
set link_win32=User32.lib winmm.lib kernel32.lib gdi32.lib shell32.lib msvcrt.lib
set link_gl=opengl32.lib
set link_cglm=cglm.lib
set link_zlib=zlib.lib
set link_jolt=Jolt.lib joltc.lib 
set link_flags=/link %link_win32% %link_gl% %link_zlib% %link_jolt% /NODEFAULTLIB:LIBCMT /LTCG

set include_dir=/I ../source/ /I ../include/ /I ../include/Jolt/
set core=../source/skate_entry.cpp

if not exist build mkdir build
pushd build
cl %build_options% %compile_flags% %core% %include_dir% %link_flags% %lib_paths% /out:%exe_name%.exe
copy ..\data\* . >NUL
popd