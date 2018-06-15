#pragma once

#include "hal/Handle/Dump.h"

namespace HMF {
namespace Handle {

struct DumpMixin {
	typedef const boost::shared_ptr<Dump> & ref_t;

	DumpMixin(ref_t dumper);

#ifndef PYPLUSPLUS
	template <typename Handle, typename ... Args>
	void dump(const char * const name, const Handle & h, Args & ... args) {
		mDumper->dump(name, h, args...);
	}
protected:
	boost::shared_ptr<Dump> dumper() const { return mDumper; }

private:
	boost::shared_ptr<Dump> mDumper;
#endif
};

} // end namespace HMF
} // end namespace Handle

