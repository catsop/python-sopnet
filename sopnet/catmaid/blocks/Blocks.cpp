#include "Blocks.h"

std::ostream&
operator<<(std::ostream& out, const Blocks& blocks) {

	out << "[";

	for (const Block& block : blocks)
		out << block;

	out << "]";

	return out;
}
