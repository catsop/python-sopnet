#include <boost/make_shared.hpp>

#include <boost/functional/hash.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include "Slice.h"

Slice::Slice(
		unsigned int id,
		unsigned int section,
		boost::shared_ptr<ConnectedComponent> component) :
	_id(id),
	_section(section),
	_component(component),
	_isWhole(true){}

unsigned int
Slice::getId() const {

	return _id;
}

unsigned int
Slice::getSection() const {

	return _section;
}

boost::shared_ptr<ConnectedComponent>
Slice::getComponent() const {

	return _component;
}

void
Slice::intersect(const Slice& other) {

	_component = boost::make_shared<ConnectedComponent>(getComponent()->intersect(*other.getComponent()));
}

void
Slice::translate(const util::point<unsigned int>& pt)
{
	_component = boost::make_shared<ConnectedComponent>(getComponent()->translate(pt));
}

bool
Slice::operator==(const Slice& other) const
{
	//TODO: == operator should be based on geometry
	return getId() == other.getId();
}

void
Slice::setWhole(bool isWhole)
{
	_isWhole = isWhole;
}


bool
Slice::isWhole() const
{
	return _isWhole;
}


/**
 * Simple hash function for Slice, over the Slice's unique id.
 */
std::size_t hash_value(const Slice& slice)
{
	return boost::hash_value(slice.getId());
}

