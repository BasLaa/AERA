//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ HUMANOBS - Replicode r_exec
//_/_/
//_/_/ Eric Nivel
//_/_/ Center for Analysis and Design of Intelligent Agents
//_/_/   Reykjavik University, Menntavegur 1, 101 Reykjavik, Iceland
//_/_/   http://cadia.ru.is
//_/_/ Copyright(c)2012
//_/_/
//_/_/ This software was developed by the above copyright holder as part of
//_/_/ the HUMANOBS EU research project, in collaboration with the
//_/_/ following parties:
//_/_/
//_/_/ Autonomous Systems Laboratory
//_/_/   Technical University of Madrid, Spain
//_/_/   http://www.aslab.org/
//_/_/
//_/_/ Communicative Machines
//_/_/   Edinburgh, United Kingdom
//_/_/   http://www.cmlabs.com/
//_/_/
//_/_/ Istituto Dalle Molle di Studi sull'Intelligenza Artificiale
//_/_/   University of Lugano and SUPSI, Switzerland
//_/_/   http://www.idsia.ch/
//_/_/
//_/_/ Institute of Cognitive Sciences and Technologies
//_/_/   Consiglio Nazionale delle Ricerche, Italy
//_/_/   http://www.istc.cnr.it/
//_/_/
//_/_/ Dipartimento di Ingegneria Informatica
//_/_/   University of Palermo, Italy
//_/_/   http://roboticslab.dinfo.unipa.it/index.php/Main/HomePage
//_/_/
//_/_/
//_/_/ --- HUMANOBS Open-Source BSD License, with CADIA Clause v 1.0 ---
//_/_/
//_/_/ Redistribution and use in source and binary forms, with or without
//_/_/ modification, is permitted provided that the following conditions
//_/_/ are met:
//_/_/
//_/_/ - Redistributions of source code must retain the above copyright
//_/_/ and collaboration notice, this list of conditions and the
//_/_/ following disclaimer.
//_/_/
//_/_/ - Redistributions in binary form must reproduce the above copyright
//_/_/ notice, this list of conditions and the following
//_/_/ disclaimer in the documentation and/or other materials provided
//_/_/ with the distribution.
//_/_/
//_/_/ - Neither the name of its copyright holders nor the names of its
//_/_/ contributors may be used to endorse or promote products
//_/_/ derived from this software without specific prior written permission.
//_/_/
//_/_/ - CADIA Clause: The license granted in and to the software under this
//_/_/ agreement is a limited-use license. The software may not be used in
//_/_/ furtherance of:
//_/_/ (i) intentionally causing bodily injury or severe emotional distress
//_/_/ to any person;
//_/_/ (ii) invading the personal privacy or violating the human rights of
//_/_/ any person; or
//_/_/ (iii) committing or preparing for any act of war.
//_/_/
//_/_/ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//_/_/ "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//_/_/ LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//_/_/ A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//_/_/ OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//_/_/ SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//_/_/ LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//_/_/ DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//_/_/ THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//_/_/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//_/_/ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef r_exec_object_h
#define r_exec_object_h

#include "../submodules/CoreLibrary/CoreLibrary/utils.h"
#include "../r_code/object.h"
#include "view.h"
#include "opcodes.h"

#include <list>


namespace r_exec {

r_exec_dll bool IsNotification(Code *object);

// Shared resources:
// views: accessed by Mem::injectNow (via various sub calls) and Mem::update.
// psln_thr: accessed by reduction cores (via overlay mod/set).
// marker_set: accessed by Mem::injectNow ans Mem::_initiate_sln_propagation.
template<class C, class U> class Object :
  public C {
private:
  size_t hash_value_;

  volatile uint32 invalidated_; // must be aligned on 32 bits.

  CriticalSection psln_thrCS_;
  CriticalSection viewsCS_;
  CriticalSection markersCS_;
protected:
  Object();
  Object(r_code::Mem *mem);
public:
  virtual ~Object(); // un-registers from the rMem's object_register.

  r_code::View *build_view(SysView *source) {

    return Code::build_view<r_exec::View>(source);
  }

  virtual bool is_invalidated();
  virtual bool invalidate(); // return false when was not invalidated, true otherwise.

  void compute_hash_value();

  float32 get_psln_thr();

  void acq_views() { viewsCS_.enter(); }
  void rel_views() { viewsCS_.leave(); }
  void acq_markers() { markersCS_.enter(); }
  void rel_markers() { markersCS_.leave(); }

  // Target psln_thr only.
  void set(uint16 member_index, float32 value);
  void mod(uint16 member_index, float32 value);

  View *get_view(Code *group, bool lock); // returns the found view if any, NULL otherwise.

  void kill();

  class Hash {
  public:
    size_t operator ()(U *o) const {

      if (o->hash_value_ == 0)
        o->compute_hash_value();
      return o->hash_value_;
    }
  };

  class Equal {
  public:
    bool operator ()(const U *lhs, const U *rhs) const { // lhs and rhs have the same hash value_, i.e. same opcode, same code size and same reference size.

      if (lhs->code(0).asOpcode() == Opcodes::Ent || rhs->code(0).asOpcode() == Opcodes::Ent)
        return lhs == rhs;

      uint16 i;
      for (i = 0; i < lhs->references_size(); ++i)
        if (lhs->get_reference(i) != rhs->get_reference(i))
          return false;
      for (i = 0; i < lhs->code_size(); ++i) {

        if (lhs->code(i) != rhs->code(i))
          return false;
      }
      return true;
    }
  };
};

// Local object.
// Used for r-code that does not travel across networks (groups and notifications) or when the rMem is not distributed.
// Markers are killed when at least one of their references dies (held by their views).
// Marker deletion is performed by registering pending delete operations in the groups they are projected onto.
class r_exec_dll LObject :
  public Object<r_code::LObject, LObject> {
public:
  static bool RequiresPacking() { return false; }
  static LObject *Pack(Code *object, r_code::Mem *mem) { return (LObject *)object; } // object is always a LObject (local operation).
  LObject(r_code::Mem *mem = NULL) : Object<r_code::LObject, LObject>(mem) {}
  LObject(r_code::SysObject *source) : Object<r_code::LObject, LObject>() {

    load(source);
  }
  virtual ~LObject() {}
};
}


#endif
