#ifndef CELLTRACKER_CELL_H__
#define CELLTRACKER_CELL_H__

#include <boost/shared_ptr.hpp>

#include <util/ProgramOptions.h>
#include <util/Hashable.h>
#include <util/point.hpp>
#include "SliceHash.h"

// forward declaration
class ConnectedComponent;

class Slice : public Hashable<Slice, SliceHash> {

public:

	/**
	 * Create a new slice.
	 *
	 * @param id The id of the new slice.
	 * @param section The section this slice lives in.
	 * @param component The blob that defines the shape of this slice.
	 */
	Slice(
			unsigned int id,
			unsigned int section,
			boost::shared_ptr<ConnectedComponent> component);

	/**
	 * Get the id of this slice.
	 */
	unsigned int getId() const;

	/**
	 * Get the section number, this slice lives in.
	 */
	unsigned int getSection() const;

	/**
	 * Get the blob of this slice.
	 */
	boost::shared_ptr<ConnectedComponent> getComponent() const;

	/**
	 * Set the wholeness flag on this slice. If set false, this slice is
	 * marked as one that has been split across a sub-image boundary.
	 * 
	 * @param isWhole true to set this as a whole slice, false otherwise
	 */
	void setWhole(bool isWhole);

	/**
	 * Retrns the wholeness flag, which indicates whether this flag is whole,
	 * or has been split across a sub-image boundary.
	 */
	bool isWhole() const;


	/**
	 * Intersect this slice with another one. Note that the result might not be
	 * a single connected component any longer.
	 */
	void intersect(const Slice& other);

	/**
	 * Translate this Slice
	 * @param pt a point representing the translation to perform.
	 */
	void translate(const util::point<int, 2>& pt);
	
	bool operator==(const Slice& other) const;
	
	bool operator<(const Slice& other) const
	{
		return _id < other._id;
	}

private:

	unsigned int _id;

	unsigned int _section;

	bool _isWhole;

	boost::shared_ptr<ConnectedComponent> _component;
};

#endif // CELLTRACKER_CELL_H__

