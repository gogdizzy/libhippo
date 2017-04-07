//
// Created by gogdizzy on 15/12/26.
//

#include <hippo/base/ScopeGuard.h>

namespace hippo {

ScopeGuard::ScopeGuard( std::function< void() > func )
	: func_( func ), dismissed_( false ) {
}

ScopeGuard::~ScopeGuard() {
	if( !dismissed_ ) {
		func_();
	}
}

void ScopeGuard::dismiss() {
	dismissed_ = true;
}

}