
#pragma once

#include <mutex>
#include <unordered_map>

namespace hippo {

template< typename K, typename V >
class SimpleConcurrentHashMap {

public:
	size_t size() {
		std::lock_guard< std::mutex >  guard( mutex_ );
		return map_.size();
	}

	bool insert( const K& key, const V& val ) {
		std::lock_guard< std::mutex >  guard( mutex_ );
		auto x = map_.insert( std::make_pair( key, val ) );
		return x.second;
	}

	bool find( const K& key, V* val = nullptr ) {
		std::lock_guard< std::mutex >  guard( mutex_ );
		auto x = map_.find( key );
		if( x == map_.end() ) return false;
		if( val != nullptr ) *val = x->second;
		return true;
	}

//	bool insert_force( K&& key, V&& val,  )

	bool erase( const K& key, V* val = nullptr ) {
		std::lock_guard< std::mutex >  guard( mutex_ );
		auto x = map_.find( key );
		if( x == map_.end() ) return false;
		if( val != nullptr ) *val = x->second;
		map_.erase( x );
		return true;
	}

	void clear() {
		std::lock_guard< std::mutex >  guard( mutex_ );
		map_.clear();
	}

private:
	std::mutex  mutex_;
	std::unordered_map< K, V >  map_;
};

}
