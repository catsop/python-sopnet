#include "Blocks.h"

std::ostream&
operator<<(std::ostream& out, const Blocks& blocks) {

	char separator = '[';

	for (const Block& block : blocks) {
		out << separator << block;
		separator = ',';
	}

	out << ']';

	return out;
}
