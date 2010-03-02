#include <assert.h>
#include "ReductionInstance.h"
#include "Object.h"
#include "Expression.h"
#include "match.h"

namespace r_exec {

using r_code::Atom;

void ReductionInstance::retain()
{
	if (referenceCount++ == 0) {
		for (int i = 0; i < references.size(); ++i) {
			if (references[i] != 0) // HACK
				references[i]->retain();
		}
	}
}

void ReductionInstance::release()
{
	if (--referenceCount == 0) {
		for (int i = 0; i < references.size(); ++i) {
			if (references[i] != 0)
				references[i]->release();
		}
		delete this;
	}
}

ReductionInstance::~ReductionInstance()
{
}

ReductionInstance::ReductionInstance(const ReductionInstance& ri)
{
	printf("copying ri %p\n", &ri);
	referenceCount = 0;
	input = ri.input;
	value = ri.value;
	copies = ri.copies;
	firstReusableCopiedObject = ri.firstReusableCopiedObject;
	references = ri.references;
	hash_value = 0;
	group = ri.group;
}

ReductionInstance* ReductionInstance::reduce(Object* input)
{
	ReductionInstance* result = new ReductionInstance(*this);
	result->referenceCount = 0;
	result->hash_value = 0;
	Expression copiedInput = input->copy(*result);
	Expression re(result);
	if (match(copiedInput, ExecutionContext(re))) {
		return result;
	} else {
		delete result;
		return 0;
	}
}

ReductionInstance* ReductionInstance::reduce(std::vector<ReductionInstance*> inputs)
{
	ReductionInstance* result = new ReductionInstance(*this);
	result->referenceCount = 0;
	result->hash_value = 0;
	// Merge the input RIs
	Expression ipgm(result);
	Expression pgm(ipgm.child(1));
	Expression inputSection = pgm.child(2);
	Expression inputsExpression = inputSection.child(1);
	int numInputs = inputsExpression.head().getAtomCount();
	assert(numInputs == inputs.size());
	for (int i = 0; i < numInputs; ++i) {
		Expression input(inputsExpression.child(i+1));
		Expression value(inputs[i]);
		value.setValueAddressing(true);
		result->merge(ExecutionContext(input), inputs[i]);
		firstReusableCopiedObject = copies.size();
	}
	
	bool allGuardsMet = true;
	// evaluate the timings
	Expression timings(inputSection.child(2));
	int numTimings = timings.head().getAtomCount();
	for (int i = 1; allGuardsMet && i <= numTimings; ++i) {
		Expression timing(timings.child(i));
		Expression result (ExecutionContext(timing).evaluate());
		if (result.head().readsAsNil()) {
			allGuardsMet = false;
		}
	}
	
	// evaluate the guards
	Expression guards(inputSection.child(3));
	int numGuards = guards.head().getAtomCount();
	for (int i = 1; allGuardsMet && i <= numGuards; ++i) {
		Expression guard(guards.child(i));
		Expression result(ExecutionContext(guard).evaluate());
		if (guard.head().readsAsNil()) {
			allGuardsMet = false;
		}
	}
	
	if (!allGuardsMet) {
		delete result;
		return 0;
	} else {
		ExecutionContext(ipgm).setResult(ipgm.head());
		ExecutionContext(ipgm).xchild(1).setResult(pgm.iptr());
		ExecutionContext(pgm).setResult(pgm.head());
		Expression productions(pgm.child(3));
		ExecutionContext(productions).setResult(productions.head());
		int numProductions = productions.head().getAtomCount();
		for (int i = 1; i <= numProductions; ++i) {
			Expression production(productions.child(i));
			ExecutionContext(production).evaluate();
		}
		return result;
	}
}

size_t ReductionInstance::ptr_hash::operator()(const ReductionInstance* ri) const
{
	return ri->hash();
}

size_t ReductionInstance::hash() const
{
	if (hash_value == 0) {
		size_t& hv = const_cast<ReductionInstance*>(this)->hash_value;
		for (int i = 0; i < input.size(); ++i)
			hv = 5 * hv + input[i].atom;
		for (int i = 0; i < value.size(); ++i)
			hv = 5 * hv + value[i].atom;
	}
	return hash_value;
}

int maximumIndex(Expression e)
{
	int numAtoms = e.head().getAtomCount();
	int result = e.getIndex() + numAtoms;
	switch(e.head().getDescriptor()) {
		case Atom::I_PTR:
		case Atom::VL_PTR:
			result = maximumIndex(e.dereference());
			break;
		case Atom::C_PTR: {
			int n = maximumIndex(e.child(1));
			if (n > result)
				result = n;
			break;
		}
		default: {
			for (int i = 1; i <= numAtoms; ++i) {
				Expression c(e.child(i));
				if (c.getIndex() > e.getIndex()) {
					int n = maximumIndex(e.child(i));
					if (n > result)
						result = n;
				}
			}
		}
	}
	return result;
}

ReductionInstance* ReductionInstance::split(ExecutionContext location)
{
	ReductionInstance* result = new ReductionInstance(group);
	int firstIndex = location.getIndex();
	int lastIndex = maximumIndex(location);
	
	// first, copy all of the atoms verbatim
	for (int n = firstIndex; n <= lastIndex; ++n) {
		result->input.push_back(input[n]);
	}
	result->syncSizes();
	
	// next, make required changes
	for (int n = 0; n < result->input.size(); ++n) {
		Atom& a = result->input[n];
		switch(a.getDescriptor()) {
			case Atom::I_PTR:
			case Atom::VL_PTR: {
				int idx = a.asIndex();
				if (idx >= firstIndex) {
					// adjust the index to point to the correct place
					if (a.getDescriptor() == Atom::I_PTR)
						a = Atom::IPointer(idx - firstIndex);
					else
						a = Atom::VLPointer(idx - firstIndex);
				} else {
					// make a copy
					if (a.getDescriptor() == Atom::I_PTR)
						a = Expression(this, a.asIndex()).copy(*result).iptr();
					else
						a = Expression(this, a.asIndex()).copy(*result).vptr();
				}
				break;
			}
			case Atom::R_PTR: {
				int old_idx = a.asIndex();
				a = Atom::RPointer(result->references.size());
				result->references.push_back(references[old_idx]);
				break;
			}
		}
	}
	CopiedObject partial;
	partial.position = 0;
	partial.object = objectForExpression(location);
	result->copies.push_back(partial);
	result->firstReusableCopiedObject = 1;
	return result;
}

void ReductionInstance::merge(ExecutionContext location, ReductionInstance* ri)
{
	syncSizes();
	
	int firstIndex = location.getIndex();
	int lastIndex = maximumIndex(location);
	
	for (int n = firstIndex; n <= lastIndex; ++n) {
		Atom& a = value[n] = ri->value[n - firstIndex];
		switch(a.getDescriptor()) {
			case Atom::I_PTR:
			case Atom::VL_PTR: {
				int idx = a.asIndex();
				if (idx <= lastIndex - firstIndex) {
					// an pointer inside the copied area.  adjust it.
					if (a.getDescriptor() == Atom::I_PTR)
						a = Atom::IPointer(idx + firstIndex);
					else
						a = Atom::VLPointer(idx + firstIndex);
				} else {
					// a pointer outside the copied area.  copy it.
					if (a.getDescriptor() == Atom::I_PTR)
						a = Expression(ri, n - firstIndex).copy(*this).iptr();
					else
						a = Expression(ri, n - firstIndex).copy(*this).vptr();
				}
				break;
			}
			case Atom::R_PTR: {
				int old_idx = a.asIndex();
				bool found = false;
				for (int i = 0; i < references.size(); ++i) {
					if (references[i] == ri->references[old_idx]) {
						a = Atom::RPointer(i);
						found = true;
						break;
					}
				}
				if (!found) {
					a = Atom::RPointer(references.size());
					references.push_back(ri->references[old_idx]);
				}
				break;
			}
		}
	}
}

void ReductionInstance::syncSizes()
{
	if (input.size() < value.size())
		input.resize(value.size());
	else
		value.resize(input.size());
}

Object* ReductionInstance::objectForExpression(Expression expr)
{
	Object* result = 0;
	for (int i = 0; i < copies.size(); ++i) {
		if (copies[i].position <= expr.getIndex())
			result = copies[i].object;
	}
	return result;
}

Object* ReductionInstance::extractObject(Expression expr)
{
	// first, check to see if the object is known to exist
	Object* result = 0;
	for (int i = 0; i < copies.size(); ++i)
		if (copies[i].position == expr.getIndex())
			return copies[i].object;
	
	ReductionInstance copyRI(group);
	expr.copy(copyRI);
	printf("creating new object\n");
	copyRI.debug();
	return Object::create(copyRI.value, copyRI.references);
}

void ReductionInstance::debug()
{
	syncSizes();
	int copyI = 0;
	for (int i = 0; i < input.size(); ++i) {
		if (copyI < copies.size() && copies[copyI].position == i) {
			printf("COPY[%d]: %p\n", copyI, copies[copyI++].object);
		}
		printf("[%02x] = input = 0x%08x value = 0x%08x\n", i, input[i].atom, value[i].atom);
	}

	for (int i = 0; i < references.size(); ++i) {
		printf("references[%d] = %p\n", i, references[i]);
	}
}

}
