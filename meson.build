project('bp-cpp', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3'])

source_files = files([
  'src/main.cpp'
])

sdl2_dep = dependency('sdl2')

exe = executable('bp', source_files,
  dependencies: [sdl2_dep], install : false)

test('basic', exe)

