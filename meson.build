project(
    'phosphor-time-manager',
    'cpp',
    version: '0.1',
    meson_version: '>=1.1.1',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        'buildtype=debugoptimized',
    ],
)

########################################################################

# Project Links

project_pretty_name = 'phosphor-time-manager'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url = project_url + '/issues/new'

summary('Issue', project_issues_url, section: 'Report Issues')

#####################################################################

# Get Compiler and default build type

compiler = meson.get_compiler('cpp')

#########################################################################

# Find the dependencies

if (get_option('tests').enabled())
        sdbusplus_dep = subproject('sdbusplus').get_variable('sdbusplus_dep')
	meson.override_dependency('sdbusplus',sdbusplus_dep)
	phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces-ami')
	phosphor_logging_dep = subproject('phosphor-logging').get_variable('phosphor_logging_dep')
else
        sdbusplus_dep = dependency('sdbusplus')
	phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
	phosphor_logging_dep = dependency('phosphor-logging')

endif
deps = [sdbusplus_dep, phosphor_logging_dep, phosphor_dbus_interfaces_dep]

###########################################################################

# Get the config data and enable options

conf_data = configuration_data()
conf_data.set('DEFAULT_TIME_MODE', get_option('default_time_mode'))
conf_data.set_quoted(
    'DEFAULT_TIME_SYNC_OBJECT_PATH',
    get_option('default_time_sync_object_path'),
)

configure_file(output: 'config.h', configuration: conf_data)


############################################################################

# Gather sources for the target binaries

phosphor_time_manager_sources = [
    'bmc_epoch.cpp',
    'manager.cpp',
    'utils.cpp',
    'settings.cpp',
]

libtimemanager = static_library(
    'libtimemanager',
    phosphor_time_manager_sources,
    dependencies: deps,
)
############################################################################

# Install the files into the build directory

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    'systemd_system_unit_dir',
    pkgconfig_define: ['prefix', get_option('prefix')],
)

filesystem = import('fs')
filesystem.copyfile('xyz.openbmc_project.Time.Manager.service',
                    install: true,
                    install_dir: systemd_system_unit_dir)

filesystem.copyfile('xyz.openbmc_project.NTPSec.Manager.service',install: true,install_dir: systemd_system_unit_dir)


#############################################################################

# Build binaries

executable('phosphor-time-manager',
           'main.cpp',
           link_with : libtimemanager,
           dependencies : deps,
           install : true)

executable('ntpsec-manager',
           'ntpsec_main.cpp',
           'ntpsec_manager.cpp',
           'ntpsec_manager.hpp',
            dependencies : deps,
           install : true)

if get_option('tests').allowed()
    subdir('test')
endif
