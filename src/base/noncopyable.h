/*
Author: gogdizzy
Date: 2015-10-09
Desc: copy from boost
*/

#pragma once

namespace hippo {

class noncopyable {

protected:

	constexpr noncopyable() = default;
	~noncopyable() = default;
	noncopyable( const noncopyable& ) = delete;
	noncopyable& operator=( const noncopyable& ) = delete;

};

} // namespace hippo
