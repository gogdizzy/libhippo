#include <hippo/util/Logging.h>

namespace hippo {

std::string g_loggerName;

void setHippoLogger( std::string name ) {
        g_loggerName = name;
}

}
