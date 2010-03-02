#include "match.h"
#include "Object.h"

namespace r_exec {

using r_code::Atom;

const Atom True = Atom::Boolean(true);
const Atom Wild = Atom::Wildcard();
const Atom TailWild = Atom::TailWildcard();


void nilSkel(ExecutionContext skel)
{
	skel.setResult(Atom::Nil());
	for (int i = 1; i <= skel.head().getAtomCount(); ++i) {
		nilSkel(skel.xchild(i));
	}
}

void copyInput(Expression input)
{
	ExecutionContext r(input);
	r.setResult(input.head());
	for (int i = 1; i <= input.head().getAtomCount(); ++i)
		copyInput(input.child(i, false));
	if (input.head().getDescriptor() == Atom::I_PTR)
		copyInput(Expression(&input.getInstance(), input.head().asIndex()));
}
	
bool matchSkel(Expression input, ExecutionContext skel)
{
	Atom s = skel.head();
	bool result = true;
	if (s != input.head() && s != Wild && s != TailWild) {
		nilSkel(skel);
		return false;
	}
	skel.setResult(input.head());
	for (int i = 1; i <= skel.head().getAtomCount(); ++i) {
		ExecutionContext skelChild = skel.xchild(i);
		if (skelChild.head() != Wild && skelChild.head() != TailWild) {
			if (!matchSkel(input.child(i), skelChild))
				result = false;
		} else {
			Expression c(input.child(i, false));
			skelChild.setResult(c.head());
			if (!c.getValueAddressing()) {
				copyInput(c);
			}
		}
	}
	return result;
}

bool match(Expression input, ExecutionContext pattern)
{
	ExecutionContext skel(pattern.xchild(1));
	ExecutionContext guards(pattern.xchild(2));

	Atom True = Atom::Boolean(true);
	bool matches = false;
	if (matchSkel(input, skel)) {
		matches = true;
		for (int i = 1; matches && i <= guards.head().getAtomCount(); ++i) {
			guards.xchild(i).evaluate();
			Expression result(guards.child(i));
			result.setValueAddressing(true);
			if (result.dereference().head().atom != True.atom)
				matches = false;
		}
	}
	return matches;
}

}
