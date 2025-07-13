executable(meson.project_name(),
           'main.c',
           'app-uri-scheme.c',
           'system-uri-scheme.c',
           'runtime-window.c',
           resources,
           dependencies : [
               dependency('gtk+-3.0'),
               dependency('webkit2gtk-4.1'),
           ],
           install : true)