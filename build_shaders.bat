@echo off

pushd build
pushd sokol_shdc
sokol-shdc --input "K:/skate/content/shader/texture_static_mesh.glsl" --output "K:/skate/source/shader/texture_static_mesh.glsl.h" --slang glsl430:hlsl5:metal_macos
popd
popd
pause