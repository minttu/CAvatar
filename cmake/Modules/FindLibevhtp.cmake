set(EVHTP_FOUND FALSE)

find_path(EVHTP_INCLUDE_DIR evhtp.h
  /usr/local/include
  /usr/include
)

find_library(EVHTP_LIBRARY evhtp
  /usr/local/lib
  /usr/lib
)

if(EVHTP_INCLUDE_DIR)
  if(EVHTP_LIBRARY)
    set(EVHTP_LIBRARIES ${EVHTP_LIBRARY})
    set(EVHTP_FOUND TRUE)
  endif(EVHTP_LIBRARY)
endif(EVHTP_INCLUDE_DIR)

if(EVHTP_FOUND)
  message(STATUS "Found libevhtp: ${EVHTP_LIBRARY}")
else(EVHTP_FOUND)
  message(FATAL_ERROR "Could NOT find libevhtp.")
endif(EVHTP_FOUND)