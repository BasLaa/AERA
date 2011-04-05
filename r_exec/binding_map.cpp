//	binding_map.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include	"mem.h"
#include	"binding_map.h"


namespace	r_exec{

	typedef	UNORDERED_MAP<Code	*,P<Code> >			Objects;
	typedef	UNORDERED_MAP<Atom,Atom>				Atoms;
	typedef	UNORDERED_MAP<Atom,std::vector<Atom> >	Structures;

	BindingMap::BindingMap():_Object(){
	}

	BindingMap::BindingMap(const	BindingMap	*source):_Object(){

		init(source);
	}

	void	BindingMap::clear(){

		objects.clear();
		atoms.clear();
		structures.clear();
	}

	void	BindingMap::init(const	BindingMap	*source){

		atoms=source->atoms;
		structures=source->structures;
		objects=source->objects;
	}

	void	BindingMap::init(Code	*source){	//	source is abstracted.

		if(source->code(0).asOpcode()==Opcodes::Var)
			objects.insert(Objects::value_type(source,source));
		else{
		
			for(uint16	i=0;i<source->code_size();++i){

				switch(source->code(i).getDescriptor()){
				case	Atom::NUMERICAL_VARIABLE:
				case	Atom::BOOLEAN_VARIABLE:
					atoms.insert(Atoms::value_type(source->code(i),source->code(i)));
					break;
				case	Atom::STRUCTURAL_VARIABLE:{
					std::vector<Atom>	v;
					for(uint16	j=0;j<=source->code(i-1).getAtomCount();++j)
						v.push_back(source->code(i-1+j));
					structures.insert(Structures::value_type(source->code(i),v));
					break;
				}
				}
			}

			for(uint16	i=0;i<source->references_size();++i)
				init(source->get_reference(i));
		}
	}

	Code	*BindingMap::bind_object(Code	*original)	const{

		if(original->code(0).asOpcode()==Opcodes::Var){

			Objects::const_iterator	o=objects.find(original);
			if(o!=objects.end()	&&	o->second!=NULL)
				return	o->second;
			else
				return	original;	//	no registered value; original is left unbound.
		}

		Code	*bound_object=_Mem::Get()->build_object(original->code(0));
		for(uint16	i=0;i<original->code_size();){	//	patch code containing variables with actual values when they exist.
			
			Atom	o=original->code(i);
			switch(o.getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:
			case	Atom::BOOLEAN_VARIABLE:{
				
				Atoms::const_iterator	_a=atoms.find(o);
				if(_a==atoms.end())	//	no registered value; original is left unbound.
					bound_object->code(i++)=o;
				else
					bound_object->code(i++)=_a->second;
				break;
			}case	Atom::STRUCTURAL_VARIABLE:{

				Structures::const_iterator	_s=structures.find(o);
				if(_s==structures.end())	//	no registered value; original is left unbound.
					bound_object->code(i++)=o;
				else{
					
					if(bound_object->code(i-1).getDescriptor()==Atom::TIMESTAMP)	//	store the tolerance in the timestamp (used to create monitoring jobs for goals).
						bound_object->code(i-1).setTimeTolerance(original->code(i-1).getTimeTolerance());
					for(uint16	j=1;j<_s->second.size();++j)
						bound_object->code(i++)=_s->second[j];
				}
				break;
			}default:
				bound_object->code(i++)=o;
				break;
			}
		}
		
		for(uint16	i=0;i<original->references_size();++i)	//	bind references when needed.
			if(needs_binding(original->get_reference(i)))
				bound_object->set_reference(i,bind_object(original->get_reference(i)));
			else
				bound_object->set_reference(i,original->get_reference(i));

		return	bound_object;
	}

	bool	BindingMap::needs_binding(Code	*original)	const{

		if(original->code(0).asOpcode()==Opcodes::Var){

			Objects::const_iterator	b=objects.find(original);
			if(b!=objects.end())
				return	true;
			else
				return	false;
		}

		for(uint16	i=0;i<original->code_size();++i){		//	find code containing variables.

			switch(original->code(i).getDescriptor()){
			case	Atom::NUMERICAL_VARIABLE:
			case	Atom::STRUCTURAL_VARIABLE:
				return	true;
			}
		}

		for(uint16	i=0;i<original->references_size();++i){

			if(needs_binding(original->get_reference(i)))
				return	true;
		}

		return	false;
	}

	Atom	BindingMap::get_atomic_variable(const	Code	*object,uint16	index){

		Atom	var;
		Atom	val=object->code(index);

		if(val.isFloat()){

			float32	tolerance=_Mem::Get()->get_float_tolerance();
			float32	min=val.asFloat()*(1-tolerance);
			float32	max=val.asFloat()*(1+tolerance);

			Atoms::const_iterator	a;
			for(a=atoms.begin();a!=atoms.end();++a)
				if(a->second.asFloat()>=min	&&
					a->second.asFloat()<=max){	//	variable already exists for the value.

					var=a->first;
					break;
				}
			
			if(!var){

				var=Atom::NumericalVariable(atoms.size(),Atom::GetFloatTolerance(tolerance));
				atoms.insert(Atoms::value_type(var,val));
			}
		}else	if(val.getDescriptor()==Atom::BOOLEAN_){

			Atoms::const_iterator	a;
			for(a=atoms.begin();a!=atoms.end();++a)
				if(a->second.asBoolean()==val.asBoolean()){	//	variable already exists for the value.

					var=a->first;
					break;
				}

			if(!var){

				var=Atom::BooleanVariable(atoms.size());
				atoms.insert(Atoms::value_type(var,val));
			}
		}

		return	var;
	}

	Atom	BindingMap::get_structural_variable(const	Code	*object,uint16	index){

		Atom	var;
		Atom	*val=&object->code(index);

		switch(val[0].getDescriptor()){
		case	Atom::TIMESTAMP:{	//	tolerance (ms) is used.
			
			uint32	tolerance_ms=_Mem::Get()->get_time_tolerance();
			uint64	tolerance_us=tolerance_ms<<10;
			uint64	t=Utils::GetTimestamp(val);
			uint64	min;
			uint64	max;
			if(t>0){

				min=t>tolerance_us?t-tolerance_us:0;
				max=t+tolerance_us;
			}else
				min=max=0;

			Structures::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(s->second[0].getDescriptor()==Atom::TIMESTAMP){

					int64	_t=Utils::GetTimestamp(&s->second[0]);
					if(_t<0)
						_t=-_t;
					if(_t>=min	&&	_t<=max){	//	variable already exists for the value.

						object->code(index).setTimeTolerance(tolerance_ms);
						var=s->first;
						break;
					}
				}
			}

			if(!var){

				object->code(index).setTimeTolerance(tolerance_ms);
				var=Atom::StructuralVariable(structures.size(),0);
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				_value[0].setTimeTolerance(tolerance_ms);
				structures.insert(Structures::value_type(var,_value));
			}
			break;
		}case	Atom::STRING:{	//	tolerance is not used.

			Structures::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(val[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=val[0].getAtomCount();++i)
						if(val[i]!=s->second[i])
							break;
					if(i==val[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}

			if(!var){

				var=Atom::StructuralVariable(structures.size(),0);
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				structures.insert(Structures::value_type(var,_value));
			}
			break;
		}case	Atom::OBJECT:{	//	tolerance is used to compare numerical structure members.

			float32	tolerance=_Mem::Get()->get_float_tolerance();

			Structures::const_iterator	s;
			for(s=structures.begin();s!=structures.end();++s){

				if(val[0]==s->second[0]){

					uint16	i;
					for(i=1;i<=val[0].getAtomCount();++i){

						if(val[i].isFloat()){

							float32	min=val[i].asFloat()*(1-tolerance);
							float32	max=val[i].asFloat()*(1+tolerance);
							if(s->second[i].asFloat()<min	||	s->second[i].asFloat()>max)
								break;
						}else	if(val[i]!=s->second[i])
							break;
					}
					if(i==val[0].getAtomCount()+1){

						var=s->first;
						break;
					}
				}
			}

			if(!var){

				var=Atom::StructuralVariable(structures.size(),Atom::GetFloatTolerance(tolerance));
				std::vector<Atom>	_value;
				for(uint16	i=0;i<=val[0].getAtomCount();++i)
					_value.push_back(val[i]);
				structures.insert(Structures::value_type(var,_value));
			}
			break;
		}
		}

		return	var;
	}

	Code	*BindingMap::get_variable_object(Code	*object){

		Code	*var=NULL;
		bool	exists=false;

		Objects::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o)
			if(o->second==object){	//	variable already exists for the value.

				var=o->first;
				break;
			}
		
		if(!var){

			var=factory::Object::Var(1);
			P<Code>	p=object;
			objects.insert(Objects::value_type(var,p));
		}

		return	var;
	}

	void	BindingMap::set_variable_object(Code	*object){

		P<Code>	original=_Mem::Get()->clone_object(object);
		object->code(0)=Atom::Object(Opcodes::Var,VAR_ARITY);	//	object is an entity. var and ent have the same size.
		objects.insert(Objects::value_type(object,original));
	}

	bool	BindingMap::match_atom(Atom	o,Atom	p){

		switch(p.getDescriptor()){
		case	Atom::NUMERICAL_VARIABLE:
			if(!o.isFloat())
				return	false;
			if(!bind_float_variable(o,p))
				return	false;
			break;
		case	Atom::BOOLEAN_VARIABLE:
			if(o.getDescriptor()!=Atom::BOOLEAN_)
				return	false;
			if(!bind_boolean_variable(o,p))
				return	false;
			break;
		default:
			if(o!=p){
			
				if(o.isFloat()	&&	p.isFloat()){

					if(abs(o.asFloat()-p.asFloat())>_Mem::Get()->get_float_tolerance())
						return	false;
				}else
					return	false;
			}
			break;
		}
		return	true;
	}

	bool	BindingMap::match_timestamp(Atom	*o,Atom	*p){

		if((p+1)->getDescriptor()==Atom::STRUCTURAL_VARIABLE){

			if(!bind_structural_variable(o,p+1))
				return	false;
		}else{

			uint64	object_time=Utils::GetTimestamp(o);
			uint64	pattern_time=Utils::GetTimestamp(p);
			uint64	time_tolerance_us=p->getTimeTolerance()<<10;
			if(object_time<pattern_time-time_tolerance_us	||	object_time>pattern_time+time_tolerance_us)
				return	false;
		}
		return	true;
	}

	bool	BindingMap::match_structure(Atom	*o,Atom	*p){

		if((p+1)->getDescriptor()==Atom::STRUCTURAL_VARIABLE){

			if(!bind_structural_variable(o,p+1))
				return	false;
		}else{

			for(uint16	i=1;i<=p->getAtomCount();++i){

				if(!match_atom(o[i],p[i]))
					return	false;
			}
		}
		return	true;
	}

	bool	BindingMap::match(Code	*object,Code	*pattern){

		if(pattern->code(0).asOpcode()==Opcodes::Var){

			if(!bind_object_variable(object,pattern))
				return	false;
		}else	if(	pattern->code(0).asOpcode()==Opcodes::Ent	||
					pattern->code(0).asOpcode()==Opcodes::Ont)
			return	object==pattern;	
		else{

			if(object->code(0)!=pattern->code(0))
				return	false;
			if(object->code_size()!=pattern->code_size())
				return	false;
			if(object->references_size()!=pattern->references_size())
				return	false;
			for(uint16	i=1;i<pattern->code_size();++i){

				switch(pattern->code(i).getDescriptor()){
				case	Atom::TIMESTAMP:
					if(!match_timestamp(&object->code(i),&pattern->code(i)))
						return	false;
					i+=2;	//	skip the rest of the structure.
					break;
				case	Atom::OBJECT:
					if(!match_structure(&object->code(i),&pattern->code(i)))
						return	false;
					i+=pattern->code(i).getAtomCount();	//	skip the rest of the structure.
					break;
				default:
					if(!match_atom(object->code(i),pattern->code(i)))
						return	false;
					break;
				}
			}

			for(uint16	i=0;i<pattern->references_size();++i){

				Code	*object_reference=object->get_reference(i);
				Code	*pattern_reference=pattern->get_reference(i);
				if(pattern_reference!=object_reference){

					if(!match(object_reference,pattern_reference))
						return	false;
				}
			}

			return	true;
		}
	}

	bool	BindingMap::bind_float_variable(Atom	val,Atom	var){

		Atoms::const_iterator	a=atoms.find(var);
		if(a->second.getDescriptor()==Atom::NUMERICAL_VARIABLE){

			atoms[var]=val;
			return	true;
		}

		float32	multiplier=var.getFloatMultiplier();
		float32	min=a->second.asFloat()*(1-multiplier);
		float32	max=a->second.asFloat()*(1+multiplier);
		if(val.asFloat()<min	||	val.asFloat()>max)	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	bool	BindingMap::bind_boolean_variable(Atom	val,Atom	var){

		Atoms::const_iterator	a=atoms.find(var);
		if(a->second.getDescriptor()==Atom::BOOLEAN_VARIABLE){

			atoms[var]=val;
			return	true;
		}

		if(val.asBoolean()!=a->second.asBoolean())	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	bool	BindingMap::bind_structural_variable(Atom	*val,Atom	*var){

		uint16	val_size=val->getAtomCount();
		Structures::iterator	s=structures.find(*var);
		if(s->second[1].getDescriptor()==Atom::STRUCTURAL_VARIABLE){

			for(uint16	i=0;i<=val_size;++i)
				s->second[i]=val[i];
			return	true;
		}
		
		switch(s->second[0].getDescriptor()){
		case	Atom::TIMESTAMP:{	//	tolerance is used.

			uint64	tolerance_us=(var-1)->getTimeTolerance()<<10;
			uint64	vt=Utils::GetTimestamp(&s->second[0]);
			uint64	min=vt-tolerance_us;
			uint64	max=vt+tolerance_us;
			uint64	t=Utils::GetTimestamp(val);
			if(t<min	||	t>max)	//	the value differs from an existing binding.
				return	false;
			return	true;
		}case	Atom::STRING:	//	tolerance is not used.
			for(uint16	i=1;i<=val_size;++i){

				if(val[i]!=s->second[i])	//	part of the value differs from an existing binding.
					return	false;
			}
			return	true;
		case	Atom::OBJECT:	//	tolerance is used to compare numerical structure members.
			for(uint16	i=1;i<=val_size;++i){

				float32	multiplier=var->getFloatMultiplier();
				if(val[i].isFloat()){

					float32	min=s->second[i].asFloat()*(1-multiplier);
					float32	max=s->second[i].asFloat()*(1+multiplier);
					float	v=val[i].asFloat();
					if(v<min	||	v>max)	//	at least one value differs from an existing binding.
						return	false;
				}else	if(val[i]!=s->second[i])	//	at least one value differs from an existing binding.
					return	false;
			}
			return	true;
		}
	}

	bool	BindingMap::bind_object_variable(Code	*val,Code	*var){

		Objects::const_iterator	o=objects.find(var);
		if(o->second->code(0).asOpcode()==Opcodes::Var){

			objects[var]=val;
			return	true;
		}

		if(o->second!=val)	//	at least one value differs from an existing binding.
			return	false;
		return	true;
	}

	void	BindingMap::copy(Code	*object,uint16	index)	const{

		uint16	args_size=objects.size()+atoms.size()+structures.size();
		object->code(index)=Atom::Set(args_size);

		uint16	code_index=index+1;
		uint16	ref_index=1;
		Objects::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o,++ref_index,++code_index){

			object->code(code_index)=Atom::RPointer(ref_index);
			object->set_reference(ref_index,o->second);
		}

		uint16	extent_index=index+args_size+1;
		Structures::const_iterator	s;
		for(s=structures.begin();s!=structures.end();++s,++code_index){

			object->code(code_index)=Atom::IPointer(extent_index);
			for(uint16	j=0;j<=s->second[0].getAtomCount();++j,++extent_index)
				object->code(extent_index)=s->second[j];
		}

		Atoms::const_iterator	a;
		for(a=atoms.begin();a!=atoms.end();++a,++code_index)
			object->code(code_index)=a->second;
	}

	void	BindingMap::load(const	Code	*object){	//	source is icst or imdl.

		uint16	var_set_index=object->code(I_HLP_ARGS).asIndex();
		uint16	var_count=object->code(var_set_index).getAtomCount();

		Objects::iterator		o=objects.begin();
		Structures::iterator	s=structures.begin();
		Atoms::iterator			a=atoms.begin();
		
		for(uint16	i=1;i<var_count;++i){

			Atom	atom=object->code(var_set_index+i);
			switch(atom.getDescriptor()){
			case	Atom::R_PTR:
				o->second=object->get_reference(atom.asIndex());
				++o;
				break;
			case	Atom::I_PTR:{
				std::vector<Atom>	val;
				uint16	s_index=atom.asIndex();
				for(uint16	j=0;j<=object->code(s_index).getAtomCount();++j)
					val.push_back(object->code(s_index+j));
				s->second=val;
				break;
			}case	Atom::NUMERICAL_VARIABLE:
			case	Atom::BOOLEAN_VARIABLE:
				a->second=atom;
				++a;
				break;
			}
		}
	}
}