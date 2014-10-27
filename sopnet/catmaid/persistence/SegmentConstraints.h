#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__

#include <map>
#include <inference/Relation.h>
#include <segments/SegmentHash.h>

class SegmentConstraint {

private:

	std::map<SegmentHash, double> _coefs;

	Relation _relation;

	double _value;

public:

	SegmentConstraint() : _relation(Equal) {}

	void setCoefficient(SegmentHash segment, double coef) {_coefs[segment] = coef;}

	void setRelation(Relation relation) {_relation = relation;}

	void setValue(double value) {_value = value;}

	const std::map<SegmentHash, double>& getCoefficients() const {return _coefs;}

	const Relation& getRelation() const {return _relation;}

	double getValue() const {return _value;}
};

typedef std::vector<SegmentConstraint> SegmentConstraints;

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__