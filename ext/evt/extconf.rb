require 'mkmf'
extension_name = 'evt_ext'
create_header
dir_config(extension_name)
create_makefile(extension_name)