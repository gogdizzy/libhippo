//
// Created by gogdizzy on 15/12/26.
//

#pragma once

#define HAVE_SPDLOG
#ifdef HAVE_SPDLOG

#include <spdlog/spdlog.h>

namespace hippo {

// typedef std::shared_ptr< spdlog::logger > Logging;
// Logging g_log;
extern std::string g_loggerName;
void setHippoLogger( std::string name );

#define HIPPO_TRACE( ... ) spdlog::get( hippo::g_loggerName )->trace( __VA_ARGS__ )
#define HIPPO_DEBUG( ... ) spdlog::get( hippo::g_loggerName )->debug( __VA_ARGS__ )
#define HIPPO_INFO( ... ) spdlog::get( hippo::g_loggerName )->info( __VA_ARGS__ )
#define HIPPO_WARN( ... ) spdlog::get( hippo::g_loggerName )->warn( __VA_ARGS__ )
#define HIPPO_ERROR( ... ) spdlog::get( hippo::g_loggerName )->error( __VA_ARGS__ )

}

#else

#define HIPPO_DEBUG( ... )
#define HIPPO_INFO( ... )
#define HIPPO_WARN( ... )
#define HIPPO_ERROR( ... )

#endif

