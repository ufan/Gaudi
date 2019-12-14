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
# - Locate Frontier_Client library
# Defines:
#
#  FRONTIER_CLIENT_FOUND
#  FRONTIER_CLIENT_INCLUDE_DIR
#  FRONTIER_CLIENT_INCLUDE_DIRS (not cached)
#  FRONTIER_CLIENT_LIBRARY
#  FRONTIER_CLIENT_LIBRARIES (not cached)
#  FRONTIER_CLIENT_LIBRARY_DIRS (not cached)

find_path(FRONTIER_CLIENT_INCLUDE_DIR frontier_client/frontier.h)
find_library(FRONTIER_CLIENT_LIBRARY NAMES frontier_client)

# handle the QUIETLY and REQUIRED arguments and set FRONTIER_CLIENT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Frontier_Client DEFAULT_MSG FRONTIER_CLIENT_INCLUDE_DIR FRONTIER_CLIENT_LIBRARY)

mark_as_advanced(FRONTIER_CLIENT_FOUND FRONTIER_CLIENT_INCLUDE_DIR FRONTIER_CLIENT_LIBRARY)

set(FRONTIER_CLIENT_INCLUDE_DIRS ${FRONTIER_CLIENT_INCLUDE_DIR})

get_filename_component(FRONTIER_CLIENT_LIBRARY_DIRS ${FRONTIER_CLIENT_LIBRARY} PATH)
set(FRONTIER_CLIENT_LIBRARIES ${FRONTIER_CLIENT_LIBRARY})
