samaya_sources = [
    'main.c',
    'samaya-application.c',
    'samaya-window.c',
    'samaya-preferences-dialog.c',
    'samaya-timer.c',
    'samaya-session.c',
    'samaya-utils.h',
]

samaya_deps = [
    dependency('gtk4'),
    dependency('libadwaita-1', version : '>= 1.7'),
    dependency('gsound'),
]

samaya_sources += gnome.compile_resources('samaya-resources', 'samaya.gresource.xml', c_name : 'samaya')

executable(
    'samaya',
    samaya_sources,
    dependencies : samaya_deps,
    install : true,
)
