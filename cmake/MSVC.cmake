MACRO ( SET_MSVC_CRT CRT )

  IF ( ${CRT} MATCHES "MT" )
    SET ( CRT_USE "MT" )
    SET ( CRT_NOT_USE "MD" )
    SET ( MSVCCRT_POSTFIX "-mt" )
  ELSE ()
    SET ( CRT_USE "MD" )
    SET ( CRT_NOT_USE "MT" )
    SET ( MSVCCRT_POSTFIX "-md" )
  ENDIF ()
  
  IF ( WIN32 )
    STRING ( REPLACE "/${CRT_NOT_USE}d" "/${CRT_USE}d" CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )

    STRING ( REPLACE "/${CRT_NOT_USE}d" "/${CRT_USE}d" CMAKE_C_FLAGS_DEBUG            "${CMAKE_C_FLAGS_DEBUG}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_C_FLAGS_MINSIZEREL       "${CMAKE_C_FLAGS_MINSIZEREL}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_C_FLAGS_RELEASE          "${CMAKE_C_FLAGS_RELEASE}" )
    STRING ( REPLACE "/${CRT_NOT_USE}"  "/${CRT_USE}"  CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO}" )
  ENDIF ()

  UNSET ( CRT_USE )
  UNSET ( CRT_NOT_USE )

ENDMACRO ( SET_MSVC_CRT )