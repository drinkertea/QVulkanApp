macro(download_and_extract _url _dir)
  string(REGEX MATCH "[^/\\?]*$" fname "${_url}")
  if(NOT "${fname}" MATCHES "(\\.|=)(bz2|7z|tar|tgz|tar\\.gz|zip)$")
    string(REGEX MATCH "([^/\\?]+(\\.|=)(bz2|7z|tar|tgz|tar\\.gz|zip))/.*$" match_result "${_url}")
    set(fname "${CMAKE_MATCH_1}")
  endif()
  if(NOT "${fname}" MATCHES "(\\.|=)(bz2|7z|tar|tgz|tar\\.gz|zip)$")
    message(FATAL_ERROR "Could not extract tarball filename from url:\n  ${_url}")
  endif()
  string(REPLACE ";" "-" fname "${fname}")

  set(DOWNLOAD_TEMP_DIR ${PROJECT_BINARY_DIR}/_downloaded)
  set(file ${DOWNLOAD_TEMP_DIR}/${fname})
  message(STATUS "file: ${file}")

  if(NOT EXISTS "${file}")
    message(STATUS "downloading...
      src='${_url}'
      dst='${file}'"
    )

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
  endif()

  message(STATUS "extracting...
    src='${file}'
    dst='${_dir}'"
  )

  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "error: file to extract does not exist: '${file}'")
  endif()

  if(NOT EXISTS "${_dir}")
    message(STATUS "Creating directory: '${_dir}'")
    file(MAKE_DIRECTORY "${_dir}")
  endif()

  message(STATUS "extracting... begin")

  execute_process(COMMAND ${CMAKE_COMMAND} -E
    tar xfz ${file}
    WORKING_DIRECTORY ${_dir}
    RESULT_VARIABLE rv
  )

  if(NOT rv EQUAL 0)
    message(FATAL_ERROR "error: extract of '${file}' failed")
  endif()

  message(STATUS "extracting... done")

  file(REMOVE_RECURSE ${DOWNLOAD_TEMP_DIR})
endmacro()
