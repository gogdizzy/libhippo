
#pragma once

#include <string>
#include <map>

namespace hippo {

class PropReader {

public:
	PropReader();
	PropReader( const std::string& filepath );

	bool readFile( const std::string& filepath );
	std::string getProp( const std::string& key );

private:
	typedef std::map< std::string, std::string >  table_t;
	table_t  table;
};

}
