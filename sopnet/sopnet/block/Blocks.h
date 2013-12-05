#ifndef BLOCKS_H__
#define BLOCKS_H__

#include <pipeline/Data.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Block.h"



class Blocks : public pipeline::Data
{
public:
	typedef std::vector<boost::shared_ptr<Block> >::iterator       iterator;

	typedef std::vector<boost::shared_ptr<Block> >::const_iterator const_iterator;
	
	Blocks();
	
	bool contains(const boost::shared_ptr<Block>& block);
	
	void add(const boost::shared_ptr<Block>& block);
	
	void remove(const boost::shared_ptr<Block>& block);
	
	const const_iterator begin() const { return _blocks.begin(); }

	iterator begin() { return _blocks.begin(); }

	const const_iterator end() const { return _blocks.end(); }

	iterator end() { return _blocks.end(); }

	unsigned int size() const { return _blocks.size(); }
	
private:
	std::vector<boost::shared_ptr<Block> > _blocks;
};

#endif //BLOCKS_H__