/*
Author: gogdizzy
Date: 2015-10-09
Desc: copy from boost
*/

#pragma once

namespace hippo {

template< typename T >
class Singleton {

public:
	static T& instance() {
		// c++11 make sure multi-thread race condition
		// if inst is not init, others will block
		static T inst;
		return inst;
	}

protected:
	Singleton() = default;
};

} // namespace hippo
