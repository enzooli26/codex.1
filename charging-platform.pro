TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += server

# Qt WebEngine is not available in the official Qt 5.15 MinGW kit on
# Windows.  Keep the legacy desktop client in builds that provide the
# module (for example Linux), but do not prevent the other applications
# from being built on a MinGW development machine.
qtHaveModule(webenginewidgets) {
    SUBDIRS += user_client
    user_client.depends = server
} else {
    message("Qt WebEngine is unavailable; skipping user_client")
}

SUBDIRS += mobile_client admin_client device_simulator
mobile_client.depends = server
admin_client.depends = server
device_simulator.depends = server
