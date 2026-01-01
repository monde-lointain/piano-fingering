# Packaging.cmake - CPack configuration

set(CPACK_PACKAGE_NAME "piano-fingering")
set(CPACK_PACKAGE_VENDOR "piano-fingering")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Piano Fingering Generator")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_INSTALL_DIRECTORY "piano-fingering")

# Platform-specific packaging
if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS")
elseif(APPLE)
  set(CPACK_GENERATOR "TGZ;DragNDrop")
else()
  set(CPACK_GENERATOR "TGZ;DEB;RPM")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "piano-fingering")
endif()

include(CPack)
