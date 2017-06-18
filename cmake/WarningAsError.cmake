#
# Toggle warnings treat as errors
#

MACRO ( SET_WARNING_AS_ERROR TOGGLE )

  IF ( ${TOGGLE} )
  
    IF ( WIN32 )
      SET ( CMAKE_C_FLAGS     "${CMAKE_C_FLAGS}      /WX /wd4819 /wd4996" )
      SET ( CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS}    /WX /wd4819 /wd4996" )
    ELSE ()
      SET ( CMAKE_C_FLAGS     "${CMAKE_C_FLAGS}      -Wall -Werror" )
      SET ( CMAKE_CXX_FLAGS   "${CMAKE_CXX_FLAGS}    -Wall -Werror" )
    ENDIF ( WIN32 )

  ELSE ()

    IF ( WIN32 )
      STRING ( REPLACE "/WX /wd4819 /wd4996" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" )
      STRING ( REPLACE "/WX /wd4819 /wd4996" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
    ELSE ()
      STRING ( REPLACE "-Wall -Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" )
      STRING ( REPLACE "-Wall -Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
    ENDIF ( WIN32 )

  ENDIF ()

ENDMACRO ( SET_WARNING_AS_ERROR )
