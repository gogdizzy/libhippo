/*
Author: gogdizzy
Date: 2015-10-09
Desc: copy from boost
*/

#pragma once

namespace hippo {

class Noncopyable {

protected:

	constexpr Noncopyable() = default;
	~Noncopyable() = default;
	Noncopyable( const Noncopyable& ) = delete;
	Noncopyable& operator=( const Noncopyable& ) = delete;

};

} // namespace hippo
