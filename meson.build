project(
  'gsound',
  'c',
  version : '1.0.3',
)

test_cflags = [
  '-fno-strict-aliasing',
  '-Wcast-align',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-Wnested-externs',
  '-Wpointer-arith',
  '-Werror=format=2',
  '-Werror=implicit-function-declaration',
  '-Werror=init-self',
  '-Werror=missing-include-dirs',
  '-Werror=missing-prototypes',
  '-Werror=pointer-arith',
  '-Werror=return-type',
  '-Wstrict-prototypes',
]

cc = meson.get_compiler('c')
warn_flags = cc.get_supported_arguments(test_cflags)
add_project_arguments(warn_flags, language: 'c')

if get_option('enable_vala')
  add_languages('vala')
endif

gio = dependency('gio-2.0')
gobject = dependency('gobject-2.0')
libcanberra = dependency('libcanberra')

gnome = import('gnome')
pkg = import('pkgconfig')

subdir('gsound')
if get_option('enable_vala')
  subdir('tools')
endif
if get_option('gtk_doc')
  subdir('docs')
endif
