# BaroboLink Robot Communications Application
# Barobo, Inc.

cmake_minimum_required(VERSION 2.8.9)

project(BaroboLink)

add_subdirectory(libstkcomms)
exec_program(cat ARGS "\"${PROJECT_SOURCE_DIR}/VERSION.TXT\"" OUTPUT_VARIABLE barobolink_version)
add_definitions(-DBAROBOLINK_VERSION=${barobolink_version})
message("Configuring BaroboLink version " ${barobolink_version})

if(CMAKE_HOST_WIN32)
  include(FindPkgConfig)
  pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
  #pkg_check_modules(PYTHON REQUIRED python)
  #find_package(PythonLibs 2.7 REQUIRED python)
  if(NOT WINSDK_INCLUDE_PATH)
    include("findwindowssdk.cmake")
  endif(NOT WINSDK_INCLUDE_PATH)
  if(NOT MSVC_INCLUDE_PATH)
    include("findmsvc.cmake")
  endif(NOT MSVC_INCLUDE_PATH)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -idirafter \"${WINSDK_INCLUDE_PATH}\" -idirafter \"${MSVC_INCLUDE_PATH}\"")

  #add_definitions(-D_MSYS -DMSYS)
  add_definitions(-D_WIN32_WINNT=0x0502 -DWINVER=0x0502 -DWIN32_LEAN_AND_MEAN)
  message( "gtk cflags: ${GTK2_CFLAGS}" )
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GTK2_CFLAGS}")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GTK2_CFLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mms-bitfields")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mms-bitfields")
  # hlh: should this append to CMAKE_C_FLAGS_RELEASE instead?
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -mms-bitfields -mwindows")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -mms-bitfields -mwindows")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -mms-bitfields")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -mms-bitfields")
elseif(APPLE)
  include(FindPkgConfig)
  pkg_check_modules(GTK2 REQUIRED gtk+-2.0 )
  pkg_check_modules(GTKMAC REQUIRED gtk-mac-integration)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")
  include_directories(${GTKMAC_INCLUDE_DIRS})
  link_directories(${GTKMAC_LIBRARY_DIRS})
else()  # linux
  set(GTK2_ADDITIONAL_SUFFIXES)
  find_package(GTK2 2.6 REQUIRED gtk glade )
#find_package(PythonLibs 2.7 REQUIRED python)
endif(CMAKE_HOST_WIN32)

if(NOT GTK2_FOUND)
  message(FATAL_ERROR "Gtk not found!")
endif(NOT GTK2_FOUND)

include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${LIBSTKCOMMS_SOURCE_DIR})
link_directories(${LIBSTKCOMMS_BINARY_DIR})
link_directories(${GTK2_LIBRARY_DIRS})
link_directories(${PYTHON_LIBRARY_DIRS})

find_package(LIBBAROBO)
include_directories(${LIBBAROBO_INCLUDE_DIRS})

##############################################################################

set(BAROBOLINK_SOURCES
    src/commsHandler.cpp
    src/configFile.cpp
    src/connectDialog.cpp
    src/connectDongleDialog.cpp
    src/controlDialog.cpp
    src/RecordMobot.cpp
    src/BaroboLink.cpp
    src/RobotManager.cpp
    src/scanMobotsDialog.cpp
    src/teachingDialog.cpp
    src/updateFirmware.cpp)
if(CMAKE_HOST_WIN32)
  list(APPEND BAROBOLINK_SOURCES BaroboLink.res)
endif()

set(BAROBOFIRMWAREUPDATE_SOURCES src/BaroboFirmwareUpdate.cpp)

add_executable(BaroboLink ${BAROBOLINK_SOURCES})
add_executable(BaroboFirmwareUpdate ${BAROBOFIRMWAREUPDATE_SOURCES})

target_link_libraries(BaroboLink ${LIBBAROBO_LIBRARIES} stkcomms pthread)
target_link_libraries(BaroboFirmwareUpdate ${LIBBAROBO_LIBRARIES} stkcomms)

if(CMAKE_COMPILER_IS_GNUCXX)
# connectDialog.cpp uses cstdint, off64_t MinGW bug requires gnu++11 instead
# of c++11
  set_target_properties(BaroboLink PROPERTIES COMPILE_FLAGS "-std=gnu++11")
endif()

set(INTERFACE_FILES
      interface/16px_move_back.png
      interface/16px_move_back.svg
      interface/16px_move_forward.png
      interface/16px_move_forward.svg
      interface/16px_stop.png
      interface/16px_stop.svg
      interface/DOF_joint_diagram.png
      interface/face_backward.svg
      interface/imobot_diagram.png
      interface/imobot_diagram.svg
      interface/iMobot.png
      interface/inch_left.svg
      interface/inch_right.svg
      interface/interface.glade
      interface/lface_forward.svg
      interface/mobotfirmwareupdateinterface.glade
      interface/move_back.png
      interface/move_back.svg
      interface/move_forward.png
      interface/move_forward.svg
      interface/rotate_left.png
      interface/rotate_left.svg
      interface/rotate_right.png
      interface/rotate_right.svg
      interface/stop.png
      interface/stop.svg)

install(FILES ${INTERFACE_FILES} DESTINATION share/BaroboLink)
install(TARGETS BaroboLink BaroboFirmwareUpdate RUNTIME DESTINATION bin)

if(CMAKE_HOST_WIN32)
  #target_link_libraries(BaroboLink ${PROJECT_SOURCE_DIR}/BaroboLink.res ${PYTHON_LIBRARIES} ${GTK2_LIBRARIES})
  target_link_libraries(BaroboLink ${PYTHON_LIBRARIES} ${GTK2_LIBRARIES})
  target_link_libraries(BaroboFirmwareUpdate ${GTK2_LIBRARIES})
elseif(APPLE)
  find_library(COREFOUNDATION_LIB CoreFoundation)
  find_library(IOKIT_LIB IOKit)
  target_link_libraries(BaroboLink ${IOKIT_LIB} ${COREFOUNDATION_LIB} ${PYTHON_LIBRARIES} ${GTK2_LIBRARIES} ${GTKMAC_LIBRARIES})
  target_link_libraries(BaroboFirmwareUpdate pthread ${IOKIT_LIB} ${COREFOUNDATION_LIB} ${GTK2_LIBRARIES} ${GTKMAC_LIBRARIES})
else()
  target_link_libraries(BaroboLink bluetooth ${GTK2_LIBRARIES} ${PYTHON_LIBRARIES})
  target_link_libraries(BaroboFirmwareUpdate bluetooth pthread ${GTK2_LIBRARIES})
  install(FILES BaroboLink.desktop DESTINATION share/applications)
endif()

if(CMAKE_SYSTEM_NAME MATCHES [Ll]inux)
  string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
  find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
  if(DPKG_PROGRAM)
       execute_process(COMMAND ${DPKG_PROGRAM} --print-architecture
                 OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                 OUTPUT_STRIP_TRAILING_WHITESPACE)
       set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_REVISION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}") 
  else(DPKG_PROGRAM)
       set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_REVISION}_${CMAKE_SYSTEM_NAME}")
  endif(DPKG_PROGRAM)

  include(InstallRequiredSystemLibraries)

  set(CPACK_SET_DESTDIR "on")
  set(CPACK_PACKAGE_NAME "BaroboLink")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
  set(CPACK_GENERATOR "DEB")

  set(VERSION 1.5.5)
  set(CPACK_DEBIAN_PACKAGE_VERSION ${VERSION})
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_SECTION "Science & Engineering")
  set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
#SET (CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgladeui-1-11, libglib2.0-0, libgtk2.0-0")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  set(CPACK_PACKAGE_DESCRIPTION "Barobo Robot Controller")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical Interface for controlling, programming, and interacting with Barobo Mobots and Linkbots.")
  set(CPACK_PACKAGE_CONTACT "david@barobo.com")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "David Ko <david@barobo.com>")
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

  set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
  include(CPack)
endif()
