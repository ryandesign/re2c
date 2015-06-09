#ifndef __LABEL__
#define __LABEL__

#include <iosfwd>

#include "src/util/c99_stdint.h"
#include "src/util/counter.h"

namespace re2c {

// label public API:
//     - get first label
//     - compare labels
//     - get label width
//     - output label to std::ostream
//
// label private API (for label counter):
//     - get initial label
//     - get next label
class label_t
{
	static const uint32_t FIRST;
	uint32_t value;
	label_t ();
	void inc ();

public:
	static label_t first ();
	bool operator < (const label_t & l) const;
	uint32_t width () const;
	friend std::ostream & operator << (std::ostream & o, label_t l);

	friend class counter_t<label_t>;
};

} // namespace re2c

#endif // __LABEL__
