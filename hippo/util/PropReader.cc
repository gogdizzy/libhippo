
#include <iostream>
#include <fstream>

#include "hippo/util/StrUtil.h"
#include "hippo/util/PropReader.h"

namespace hippo {

PropReader::PropReader() {
}

PropReader::PropReader( const std::string& filepath ) {
	readFile( filepath );
	
}

bool PropReader::readFile( const std::string& filepath ) {
	std::ifstream f( filepath );
	if( !f.is_open() ) {
		std::cerr << "Can't open file " << filepath << std::endl;
		return false;
	}

	char tmp[512];
	while( !f.eof() ) {
		f.getline( tmp, sizeof(tmp) );
		std::string s = StrUtil::trim( tmp );
		if( s.size() < 2 ) continue;
		if( s[0] == '#' || ( s[0] == '/' && s[1] == '/' ) ) continue;
		auto pos = s.find( '=' );
		if( pos == std::string::npos ) continue;
		std::string key = StrUtil::trim( s.substr( 0, pos ) );
		std::string val = StrUtil::trim( s.substr( pos + 1 ) );
		table.insert( std::make_pair( key, val ) );
	}

	f.close();
	return true;
}

std::string PropReader::getProp( const std::string& key ) {
	auto it = table.find( key );
	if( it == table.end() ) return "";
	return it->second;
}

}
