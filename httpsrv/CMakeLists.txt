set(SRC
	srv.cpp

	${PUBLIC_DIR}/httpsrv/srv.h

	httplib.h

	tinyxml2.cpp
	tinyxml2.h
)

add_library(httpsrv STATIC ${SRC})
target_include_directories(httpsrv PUBLIC ${PUBLIC_DIR})