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
# - Locate Intel Amplifier toolkit
# Defines:
#
#  INTELAMPLIFIER_FOUND
#  INTELAMPLIFIER_INCLUDE_DIRS
#  INTELAMPLIFIER_LIBRARIES
#  INTELAMPLIFIER_LIB_DIRS
#  INTELAMPLIFIER_AMPLEXE_CL_EXECUTABLE
#  INTELAMPLIFIER_LIBITTNOTIFY
# 
# INTELAMPLIFIER_INSTALL_DIR will searched first to find the paths
# Otherwise it will try to search for the amplxe-cl in the ENV{PATH}
# and set other directories wrt amplxe-cl. 
# INTELAMPLIFIER_INSTALL_DIR shoudl point to installation root of the package
#
#  All blame goes to
#  Sami Kama<sami_dot_kama_at_cern_dot_ch> Apr 2016
#

if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
set(INTELBINSUFFIX "64")
else()
set(INTELBINSUFFIX "32")
endif()

if(${INTELAMPLIFIER_INSTALL_DIR})
  find_path(INTELAMPLIFIER_INCLUDE_DIR ittnotify.h HINTS ${INTELAMPLIFIER_INSTALL_DIR} PATH_SUFFIXES "include")
  if(INTELAMPLIFIER_INCLUDE_DIR)
    find_program(INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE amplxe-cl HINTS ${INTELAMPLIFIER_INSTALL_DIR} PATH_SUFFIXES "bin${INTELBINSUFFIX}" "." )
    set(INTELAMPLIFIER_INCLUDE_DIRS "${INTELAMPLIFIER_INSTALL_DIR}/include")
    find_library(INTELAMPLIFIER_LIBITTNOTIFY ittnotify HINTS ${INTELAMPLIFIER_INSTALL_DIR} PATH_SUFFIXES "lib${INTELBINSUFFIX}")    
    get_filename_component(INTELAMPLIFIER_LIB_DIRS "${INTELAMPLIFIER_LIBITTNOTIFY}" DIRECTORY)
  endif()
else()
  find_program(INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE amplxe-cl HINTS ${INTELAMPLIFIER_INSTALL_DIR} PATH_SUFFIXES "bin${INTELBINSUFFIX}" "." )
  if(INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE)
    get_filename_component(INTELAMPLIFIER_AMPLXE_INST_BASE "${INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE}" DIRECTORY)
    get_filename_component(INTELAMPLIFIER_AMPLXE_INST_BASE "${INTELAMPLIFIER_AMPLXE_INST_BASE}" DIRECTORY)
    find_path(INTELAMPLIFIER_INCLUDE_DIR ittnotify.h PATHS "${INTELAMPLIFIER_AMPLXE_INST_BASE}/include")    
    set(INTELAMPLIFIER_INCLUDE_DIRS "${INTELAMPLIFIER_INCLUDE_DIR}")
    find_library(INTELAMPLIFIER_LIBITTNOTIFY ittnotify HINTS ${INTELAMPLIFIER_AMPLXE_INST_BASE} PATH_SUFFIXES "lib${INTELBINSUFFIX}")
    get_filename_component(INTELAMPLIFIER_LIB_DIRS "${INTELAMPLIFIER_LIBITTNOTIFY}" DIRECTORY)
  endif()
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(IntelAmplifier FOUND_VAR INTELAMPLIFIER_FOUND 
  REQUIRED_VARS INTELAMPLIFIER_INCLUDE_DIRS INTELAMPLIFIER_LIBITTNOTIFY INTELAMPLIFIER_LIB_DIRS INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE)
mark_as_advanced(INTELAMPLIFIER_FOUND INTELAMPLIFIER_INCLUDE_DIRS INTELAMPLIFIER_AMPLXE_CL_EXECUTABLE INTELAMPLIFIER_LIBITTNOTIFY INTELAMPLIFIER_LIB_DIRS )
