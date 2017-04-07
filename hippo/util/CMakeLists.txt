set( util_SRCS
	Logging.cc
	PropReader.cc
	RandUtil.cc
	StringPiece.cc
	StrUtil.cc
)

add_library( hippoutil ${util_SRCS} )
