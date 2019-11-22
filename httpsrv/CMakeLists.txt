set(SRC
	httplib.h
	srv.cpp

	${PUBLIC_DIR}/httpsrv/srv.h
)

add_library(httpsrv STATIC ${SRC})
target_include_directories(httpsrv PUBLIC ${PUBLIC_DIR})