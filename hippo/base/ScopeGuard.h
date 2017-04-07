//
// Created by gogdizzy on 15/12/26.
//

#include <functional>

#include <hippo/base/Noncopyable.h>

namespace hippo {

class ScopeGuard : Noncopyable {

public:
	explicit ScopeGuard( std::function<void()> func );
	~ScopeGuard();
	void dismiss();

private:
	std::function<void()> func_;
	bool dismissed_;
};

}
