samaya_sources = [
  'main.c',
  'samaya-application.c',
  'samaya-window.c',
  'timer.c',
  'session.c'
]

samaya_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.4'),
]

samaya_sources += gnome.compile_resources('samaya-resources',
  'samaya.gresource.xml',
  c_name: 'samaya'
)

executable('samaya', samaya_sources,
  dependencies: samaya_deps,
       install: true,
)
