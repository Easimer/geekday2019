set(SRC
	httplib.h
	srv.cpp

	${PUBLIC_DIR}/srv.h
)

add_library(httpsrv STATIC ${SRC})