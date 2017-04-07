
#pragma once

#include <hippo/util/StringPiece.h>

namespace hippo {

struct Serializable {
	virtual bool from( StringPiece buf ) = 0;
	virtual bool to( std::string &buf ) const = 0;
};

}
