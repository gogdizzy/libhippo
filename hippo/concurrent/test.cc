#include "SimpleConcurrentHashMap.h"
#include <stdio.h>
int main() {
	int p = 3, q = 4;
	hippo::SimpleConcurrentHashMap< int, int > hm;
	hm.insert( 3, 5 );
	hm.insert( p, q );
	hm.erase( 3, &q );
	printf( "%d %d\n", q, hm.size() );
	return 0;
}
