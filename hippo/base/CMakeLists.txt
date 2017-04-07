set( base_SRCS
    ScopeGuard.cc
    Timestamp.cc
        ../codec/CommandRegistry.h)
add_library( hippobase ${base_SRCS} )