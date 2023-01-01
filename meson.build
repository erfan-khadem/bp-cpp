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

exe = executable('bp', source_files,
  dependencies: [sdl2_dep, sdl2_mixer_dep, sdl2_image_dep], install : false)

test('basic', exe)

