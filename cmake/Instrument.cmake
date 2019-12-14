#####################################################################################
# (c) Copyright 1998-2019 CERN for the benefit of the LHCb and ATLAS collaborations #
#                                                                                   #
# This software is distributed under the terms of the Apache version 2 licence,     #
# copied verbatim in the file "LICENSE".                                            #
#                                                                                   #
# In applying this licence, CERN does not waive the privileges and immunities       #
# granted to it by virtue of its status as an Intergovernmental Organization        #
# or submit itself to any jurisdiction.                                             #
#####################################################################################
#
# Include this file at the end of your cmake file and declare the macros/functions you want to instrument
# Instrumentation will generate lines starting with 'TIMING : ' that can then be plugged into the
# cmakeLogToCacheGrind.py script for nice displaying in kcachegrind
#

macro(GETTIME RESULT)
    execute_process(COMMAND "date" "+%s%N" OUTPUT_VARIABLE ${RESULT})
endmacro()

# Instrument a given function
macro(instrument func)
  # Need to make func a "proper" variable since we're in a macro
  set(func ${func})
  set(OutputFile ${CMAKE_BINARY_DIR}/instrument/instrument_${func}.cmake)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/instrument_func.cmake.in ${OutputFile} @ONLY)
  include(${OutputFile})
endmacro()

# Instrument a given function with different names depending on the value of tis first parameter
macro(instrument_1starg func)
  # Need to make func a "proper" variable since we're in a macro
  set(func ${func})
  set(arg "_$")
  set(arg "${arg}{ARGV0}")
  set(OutputFile ${CMAKE_BINARY_DIR}/instrument/instrument_${func}.cmake)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/instrument_func.cmake.in ${OutputFile} @ONLY)
  include(${OutputFile})
endmacro()

# full instrumentation of Gaudi Cmake
instrument(gaudi_collect_subdir_deps)
instrument(gaudi_list_dependencies)
instrument(gaudi_subdir)
instrument(gaudi_get_package_name)
instrument(gaudi_global_target_append)
instrument(gaudi_global_target_get_info)
instrument(gaudi_expand_sources)
instrument(gaudi_common_add_build)
instrument(gaudi_add_genheader_dependencies)
instrument(gaudi_linker_library)
instrument(gaudi_component_library)
instrument(gaudi_install_cmake_modules)
instrument(gaudi_generate_project_config_version_file)
instrument(gaudi_generate_project_config_file)
instrument(gaudi_generate_project_platform_config_file)
instrument(gaudi_external_project_environment)
instrument(gaudi_find_data_package)
instrument(gaudi_resolve_include_dirs)
instrument(gaudi_depends_on_subdirs)
instrument(gaudi_sort_subdirectories)
instrument(gaudi_get_packages)
instrument(gaudi_resolve_link_libraries)
instrument(gaudi_merge_files_append)
instrument(gaudi_merge_files)
instrument(gaudi_generate_configurables)
instrument(gaudi_generate_confuserdb)
instrument(gaudi_get_required_include_dirs)
instrument(gaudi_get_required_library_dirs)
instrument(gaudi_get_genheader_targets)
instrument(gaudi_add_library)
instrument(gaudi_add_module)
instrument(gaudi_add_dictionary)
instrument(gaudi_add_python_module)
instrument(gaudi_add_executable)
instrument(gaudi_add_unit_test)
instrument(gaudi_add_test)
instrument(gaudi_install_headers)
instrument(gaudi_install_python_modules)
instrument(gaudi_install_scripts)
instrument(gaudi_alias)
instrument(gaudi_install_joboptions)
instrument(gaudi_install_resources)
instrument(gaudi_generate_componentslist)
instrument(gaudi_env)
instrument(gaudi_build_env)
instrument(gaudi_export)
instrument(gaudi_generate_project_manifest)
instrument_1starg(find_package)
