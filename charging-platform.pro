TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += server user_client mobile_client admin_client device_simulator
user_client.depends = server
mobile_client.depends = server
admin_client.depends = server
device_simulator.depends = server
