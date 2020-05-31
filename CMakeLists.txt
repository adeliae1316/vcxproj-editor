cmake_minimum_required(VERSION 3.14)

set( TARGET vcxproj_editor )
project( ${TARGET} CXX )

set ( SOURCES 
    ${TARGET}.cpp
)

set ( HEADERS

)

add_executable( ${TARGET}
    ${SOURCES}
    ${HEADERS}
)
