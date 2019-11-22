set(SRC
	srv.cpp

	${PUBLIC_DIR}/httpsrv/srv.h

	${PUBLIC_DIR}/httplib.h
)

add_library(httpsrv STATIC ${SRC})
target_include_directories(httpsrv PUBLIC ${PUBLIC_DIR})