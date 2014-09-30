#include "Blocks.h"

std::ostream&
operator<<(std::ostream& out, const Blocks& blocks) {

	out << "[";

	foreach (const Block& block, blocks)
		out << "(" << block.x() << ", " << block.y() << ", " << block.z() << ")";

	out << "]";

	return out;
}
