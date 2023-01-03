project('bp-cpp', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3'])

source_files = files([
  'src/main.cpp',
  'src/music-player.cpp'
])

sdl2_dep = dependency('sdl2')
sdl2_mixer_dep = dependency('sdl2_mixer')
sdl2_image_dep = dependency('sdl2_image')

imgui_incdir = include_directories('external/imgui')
imgui_backend_incdir = include_directories('external/imgui/backends')

imgui_src = files([
  'external/imgui/imgui.cpp',
  'external/imgui/imgui_draw.cpp',
  'external/imgui/imgui_tables.cpp',
  'external/imgui/imgui_widgets.cpp',
  'external/imgui/backends/imgui_impl_sdl.cpp',
  'external/imgui/backends/imgui_impl_sdlrenderer.cpp'
])

source_files += imgui_src

exe = executable('bp', source_files,
  dependencies: [sdl2_dep, sdl2_mixer_dep, sdl2_image_dep],
  include_directories: [imgui_incdir, imgui_backend_incdir],
  install : false)

test('basic', exe)

