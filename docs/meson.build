
doc_module = meson.project_name()

conf = configuration_data()
conf.set('VERSION', meson.project_version())

configure_file(
  input: 'version.xml.in',
  output: 'version.xml',
  configuration: conf
)

gnome.gtkdoc(
  doc_module,
  module_version: meson.project_version(),
  main_sgml: 'gsound-docs.sgml',
  src_dir: gsound_includes,
  dependencies: gsound_dep,
  scan_args: '--rebuild-types',
  gobject_typesfile: doc_module + '.types',
  install: true
)
