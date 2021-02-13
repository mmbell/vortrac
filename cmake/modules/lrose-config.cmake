# User can tell us where to look

set(LROSE_PREFIX $ENV{LROSE_INSTALL_DIR})

# If not, use the default location
if(NOT LROSE_PREFIX)
  set(LROSE_PREFIX "/usr/local/lrose")
endif(NOT LROSE_PREFIX)

set(LROSE_LIBRARIES kd tdrp Radx Ncxx toolsa dataport netcdf hdf5 z)

# Add a custom generator for TDRP Params.cc and Params.hh files
# from their associated paramdef.<app> file

find_program(TDRP_EXECUTABLE tdrp_gen PATHS ${LROSE_PREFIX}/bin /usr/local/bin)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Params.hh ${CMAKE_CURRENT_SOURCE_DIR}/Params.cc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/paramdef.${PROJECT_NAME}
  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ${TDRP_EXECUTABLE}
            -c++
            -f paramdef.${PROJECT_NAME}
            -prog ${PROJECT_NAME}
            -add_ncar_copyright
  COMMENT "Generating/updating Params.hh and Params.cc for ${PROJECT_NAME}"
  )


message(STATUS "lrose-config: CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})
message(STATUS "lrose-config: LROSE_PREFIX: " ${LROSE_PREFIX})
message(STATUS "lrose-config: TDRP_EXECUTABLE: " ${TDRP_EXECUTABLE})

