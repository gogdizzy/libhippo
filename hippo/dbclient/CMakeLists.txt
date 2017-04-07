set( dbclient_SRCS
	RedisClient.cc
)

add_library( hippodbclient ${dbclient_SRCS} )
target_link_libraries( hippodbclient hiredis )
target_link_libraries( hippodbclient hippoutil )
