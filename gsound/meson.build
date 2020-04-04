
gsound_ns = 'GSound'
gsound_ns_ver = '1.0'

gsound_h = 'gsound.h'

gsound_headers = files(
  gsound_h,
  'gsound-attr.h',
  'gsound-context.h',
)

gsound_sources = files(
  'gsound-context.c',
)

gsound_includes = include_directories('.')

gsound_dependencies = [gobject, gio, libcanberra]

gsound_lib = library(
  meson.project_name(),
  gsound_sources,
  dependencies: gsound_dependencies,
  soversion: '0',
  version: '0.0.2',
  install: true,
)

gsound_dep = declare_dependency(
  include_directories: gsound_includes,
  dependencies: gsound_dependencies,
  link_with: gsound_lib,
)

install_headers(gsound_headers)

pkg.generate(
  libraries: gsound_lib,
  name: meson.project_name(),
  version: meson.project_version(),
  description: 'GObject wrapper for libcanberra',
)

if get_option('introspection')
  gir = gnome.generate_gir(
    gsound_lib,
    sources: gsound_headers + gsound_sources,
    dependencies: gsound_dependencies,
    header: gsound_h,
    namespace: gsound_ns,
    nsversion: gsound_ns_ver,
    identifier_prefix: gsound_ns,
    symbol_prefix: gsound_ns.to_lower(),
    includes: ['GObject-2.0', 'Gio-2.0'],
    install: true,
  )

  if get_option('enable_vala')
    gsound_vapi = gnome.generate_vapi(
      gsound_ns.to_lower(),
      sources: gir[0],
      packages: ['gobject-2.0', 'gio-2.0'],
      install: true,
  )
  endif
endif
