#include <stdio.h>
#include "Expression.h"
#include "Object.h"

namespace r_exec {

using r_code::Atom;

Expression Expression::dereference() const
{
    Expression r(*this);
    switch(head().getDescriptor()) {
        case Atom::I_PTR:
            r.index = head().asIndex();
            return r.dereference();
        case Atom::VL_PTR:
            r.index = head().asIndex();
            r.setValueAddressing(true);
            return r.dereference();
		case Atom::C_PTR: {
			Expression p(r.child(1).dereference());
			for (int16 i = 2; i <= r.head().getAtomCount(); ++i) {
				int16 idx = r.child(i, false).head().asIndex();
				if (idx > p.head().getAtomCount()) {
					printf("out of bounds chain pointer index %d:%d (max %d)\n", i, idx, p.head().getAtomCount());
				} else {
					p = p.child(idx).dereference();
				}
			}
			return p;
		}
        case Atom::R_PTR: {
			uint16 idx = head().asIndex();
			Object* o = instance->references[idx];
			if (o) {
				for (int i = instance->firstReusableCopiedObject; i < instance->copies.size(); ++i) {
					if (instance->copies[i].object == o && !instance->copies[i].isView)
						return Expression(instance, instance->copies[i].position);
				}
				return o->copy(*instance);
			} else {
				int objectIndex = 0;
				for (int i = 0; i < instance->copies.size(); ++i) {
					if (instance->copies[i].position > index)
						break;
					if (!instance->copies[i].isView)
						objectIndex = instance->copies[i].position;
				}
				return Expression(instance, objectIndex);
			}
		}
        case Atom::THIS:
			return Expression(instance, 0);
		case Atom::VIEW: {
			Object* o = instance->objectForExpression(*this);
			Group* g = instance->getGroup();
			return o->copyVisibleView(*instance, g);
		}
		case Atom::MKS: {
			Object* o = instance->objectForExpression(*this);
			return o->copyMarkerSet(*instance);
		}
		case Atom::VWS: {
			Object* o = instance->objectForExpression(*this);
			return o->copyViewSet(*instance);
		}
        default:
            return r;
    }
}

Expression Expression::copy(ReductionInstance& dest) const
{
	//printf("Expression: copying %p(%x,%d) -> %p\n", instance, index, isValue, &dest);

	dest.syncSizes();
	// create an empty reference that will become the result
	Expression result(&dest, dest.value.size(), true);

	// First, check to see if this expression is an exact copy of an existing object
	for (int i = 0; i < instance->copies.size(); ++i) {
		if (instance->copies[i].position == index && !instance->copies[i].isView) {
			Object *o = instance->copies[i].object;
			int j = dest.getReferenceIndex(o);
			dest.value.push_back(Atom::RPointer(j));
			return result;
		}
	}
			
	// copy the top-level expression.  At this point it will have back-links
	dest.value.push_back(head());
	for (int i = 1; i <= head().getAtomCount(); ++i) {
		dest.value.push_back(child(i, false).head());
	}

	// copy any children which reside in the value array
	if (result.head().getDescriptor() != Atom::TIMESTAMP) {
		for (int i = 0; i <= result.head().getAtomCount(); ++i) {
			Expression rc(result.child(i, false));
			Expression c(child(i, false));
			switch(c.head().getDescriptor()) {
				case Atom::I_PTR:
					c = Expression(instance, c.head().asIndex(), isValue);
					rc.head() = c.copy(dest).iptr();
					break;
				case Atom::VL_PTR:
					c = Expression(instance, c.head().asIndex(), true);
					rc.head() = c.copy(dest).iptr();
					break;
				case Atom::R_PTR:
					if (&dest != instance) {
						rc.head() = Atom::RPointer(dest.getReferenceIndex(instance->references[c.head().asIndex()]));
					}
					break;
			}
		}
	}
	dest.syncSizes();
	//printf("copied %p(%x)\n", &dest, result.index);
	return result;
}

int16 Expression::getReferenceIndex()
{
	if (isValue || (head().getDescriptor() != Atom::OBJECT && head().getDescriptor() != Atom::MARKER))
		return -1;
	for (int i = 0; i < instance->copies.size(); ++i) {
		if (instance->copies[i].position == index && !instance->copies[i].isView)
			return instance->getReferenceIndex(instance->copies[i].object);
	}
	return -1;
}

}
