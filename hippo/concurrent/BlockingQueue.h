//
// Created by gogdizzy on 16/1/24.
//

#pragma once


namespace hippo {

template < typename T >
class BlockingQueue {

public:
	virtual bool put( const T& ) = 0;
	virtual T take() = 0;
	virtual bool poll( T& v, int64_t timeout_nanos ) = 0;
	virtual size_t size() const = 0;
};

}
