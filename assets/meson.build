resource_files = files(
    'js/global.js',
    'style/global.css',
    'gresource.xml'
)

gnome = import('gnome')
resources = gnome.compile_resources(
    'app-resources',
    'gresource.xml',
    source_dir : 'resources',
    c_name : 'app',
    export : true,
)