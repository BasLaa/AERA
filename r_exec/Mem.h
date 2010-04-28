/*
 *  Mem.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 21/12/2009.
 *
 *
 * The organization of the system is performed by the Mem.  Specifically:
 *  * manage the objects and groups
 *  * decay of activation and saliency
 *  * create notifications about changed and/or extreme control values
 *  * inform the RCore about programs becoming active or inactive
 *  * inform the RCore about objects (local or visible) becoming salient
 *  * interface with the RSystem for I/O and functions not related
 *    to the object store
 *
 * The Mem interface is thread-safe.
 *
 * The Mem runs asynchronously, between calls to resume() and suspend().
 * The Mem assumes ownership of objects passed to it, and retains ownership
 * of objects passsed from it -- such objects may only be accesed
 *  * while the Mem is suspended
 *  * during calls to output->receive()
 */
 
#ifndef __MEM_H
#define __MEM_H
#include <vector>
#include "fwd.h"
#include "Asynchronous.h"
#include "ObjectReceiver.h"
#include "../r_code/object.h"

namespace r_exec {

// BUGS: suspend/resume doesn't work; simplify or ask eric for assist
class dll_export	Mem : public ObjectReceiver, public Asynchronous {
public:
	static Mem* create(
		int64 resilienceUpdatePeriod,
		int64 baseUpdatePeriod,
		UNORDERED_MAP<std::string, r_code::Atom> classes,
		std::vector<r_code::Object*> objects,
		ObjectReceiver* output
	);
	virtual ~Mem() {}
};

}

#endif // __MEM_H
