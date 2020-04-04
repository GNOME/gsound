project(
  'gsound',
  'c',
  version : '1.0.2',
)
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
