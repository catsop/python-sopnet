#include <boost/make_shared.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <iostream>
#include "Slice.h"

Slice::Slice(
		unsigned int id,
		unsigned int section,
		boost::shared_ptr<ConnectedComponent> component) :
	_id(id),
	_section(section),
	_isWhole(true),
	_component(component) {}

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
	setHashDirty();
}

void
Slice::translate(const util::point<int, 2>& pt)
{
	_component = boost::make_shared<ConnectedComponent>(getComponent()->translate(pt));
	setHashDirty();
}

bool
Slice::operator==(const Slice& other) const
{
	return getSection() == other.getSection() && (*getComponent()) == (*other.getComponent());
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
