//
// Created by gogdizzy on 15/12/26.
//

#include <string.h>
#include <string>
#include <hippo/util/StringPiece.h>

namespace hippo {

StringArg::StringArg( const char *str )
		: str_( str ) {

}

StringArg::StringArg( const string &str )
		: str_( str.c_str() ) {

}

StringPiece::StringPiece()
		: ptr_( nullptr ), length_( 0 ) {

}

StringPiece::StringPiece( const char *str )
		: ptr_( str ), length_( static_cast<int>( strlen( ptr_ )) ) {

}

StringPiece::StringPiece( const unsigned char *str )
		: ptr_( reinterpret_cast<const char *>(str) ),
		  length_( static_cast<int>( strlen( ptr_ )) ) {

}

StringPiece::StringPiece( const string &str )
		: ptr_( str.data() ), length_( static_cast<int>( str.size() ) ) {

}

StringPiece::StringPiece( const char *offset, int len )
		: ptr_( offset ), length_( len ) {

}

}

