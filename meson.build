project('bp-cpp', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3'])

source_files = files([
  'src/main.cpp'
])

exe = executable('bp', source_files,
  install : false)

test('basic', exe)

