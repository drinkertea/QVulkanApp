macro(get_qt _version)
  set(QT_DIR ${PROJECT_BINARY_DIR}/Qt)
  if (EXISTS ${PROJECT_BINARY_DIR}/Qt)
    message(STATUS "Qt already downloaded at: ${PROJECT_BINARY_DIR}/Qt, to force download remove this directory.")
  else()

  string(REPLACE "." "" no_dot_ver ${_version})
  message(STATUS "REPLACE: ${no_dot_ver}")
  if (WIN32)
    set(qt_platform "windows_x86")
    set(qt_unpacked "msvc2019_64")
    set(qt_compiler "win64_${qt_unpacked}")
  else()
    set(qt_platform "linux_x64")
    set(qt_compiler "gcc_64")
  endif()
  set(_url "http://download.qt.io/online/qtsdkrepository/${qt_platform}/desktop/qt5_${no_dot_ver}/qt.qt5.${no_dot_ver}.${qt_compiler}")
  message(STATUS "url: ${_url}")

  set(DOWNLOAD_TEMP_DIR "_downloaded")
  set(file ${PROJECT_BINARY_DIR}/${DOWNLOAD_TEMP_DIR}/page.html)
  message(STATUS "file: ${file}")

  file(DOWNLOAD
    ${_url}
    ${file}
    STATUS status
    SHOW_PROGRESS
  )

  list(GET status 0 status_code)
  list(GET status 1 status_string)

  if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "error: downloading '${_url}' failed
      status_code: ${status_code}
      status_string: ${status_string}
      log: ${log}
    ")
  endif()

  file(STRINGS ${file} qt_links)
  foreach(qt_link ${qt_links})
    string(REGEX MATCH "\"${_version}.*qtbase.*7z\"" _ "${qt_link}")
    set(fname ${CMAKE_MATCH_0})
    if("${fname}" STREQUAL "")
      continue()
    endif()
    message(STATUS "File found: ${fname}")
    break()
  endforeach()

  string(REPLACE "\"" "" fname ${fname})

  include(${PROJECT_SOURCE_DIR}/cmake/download_and_extract.cmake)
  download_and_extract(${_url}/${fname} ${PROJECT_BINARY_DIR}/QtTemp)

  file(RENAME ${PROJECT_BINARY_DIR}/QtTemp/${_version}/${qt_unpacked} ${PROJECT_BINARY_DIR}/Qt)

  file(REMOVE_RECURSE ${PROJECT_BINARY_DIR}/QtTemp)
  endif()
endmacro()
