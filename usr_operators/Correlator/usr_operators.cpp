//	usr_operators.cpp
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

#include	"usr_operators.h"
#include	"correlator.h"

#include	"../r_exec/init.h"
#include	"../r_exec/mem.h"
#include	"../r_comp/decompiler.h"

#include	<iostream>
#include	<cmath>


uint16	Vec3Opcode;

////////////////////////////////////////////////////////////////////////////////

bool	add(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()+rhs.getChild(1)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()+rhs.getChild(2)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()+rhs.getChild(3)[0].asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	sub(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()-rhs.getChild(1)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()-rhs.getChild(2)[0].asFloat()));
		context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()-rhs.getChild(3)[0].asFloat()));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	mul(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].isFloat()){

		if(rhs[0].asOpcode()==Vec3Opcode){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(1))[0].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(2))[0].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs[0].asFloat()*(*rhs.getChild(3))[0].asFloat()));
			return	true;
		}
	}else	if(lhs[0].asOpcode()==Vec3Opcode){

		if(rhs[0].isFloat()){

			index=context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(1))[0].asFloat()-rhs[0].asFloat()));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(2))[0].asFloat()-rhs[0].asFloat()));
			context.addCompoundResultPart(Atom::Float((*lhs.getChild(3))[0].asFloat()-rhs[0].asFloat()));
			return	true;
		}
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	dis(const	r_exec::Context	&context,uint16	&index){

	r_exec::Context	lhs=*context.getChild(1);
	r_exec::Context	rhs=*context.getChild(2);

	if(lhs[0].asOpcode()==Vec3Opcode	&&	rhs[0].asOpcode()==Vec3Opcode){

		float32	d1=(*lhs.getChild(1))[0].asFloat()-(*rhs.getChild(1))[0].asFloat();
		float32	d2=(*lhs.getChild(2))[0].asFloat()-(*rhs.getChild(2))[0].asFloat();
		float32	d3=(*lhs.getChild(3))[0].asFloat()-(*rhs.getChild(3))[0].asFloat();

		float32	norm2=d1*d1+d2*d2+d3*d3;
		index=context.setAtomicResult(Atom::Float(sqrt(norm2)));
		return	true;
	}

	index=context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

void	Init(OpcodeRetriever	r){

	const	char	*vec3="vec3";

	Vec3Opcode=r(vec3);

	std::cout<<"usr operators initialized"<<std::endl;
}

uint16	GetOperatorCount(){

	return	4;
}

void	GetOperatorName(char	*op_name){

	static	uint16	op_index=0;

	if(op_index==0){

		std::string	s="add";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==1){

		std::string	s="sub";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==2){

		std::string	s="mul";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}

	if(op_index==3){

		std::string	s="dis";
		memcpy(op_name,s.c_str(),s.length());
		++op_index;
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

//	Sample c++ user-defined program.
class	TestController:
public	r_exec::Controller{
private:
	float32	arg1;
	bool	arg2;
public:
	TestController(r_exec::_Mem	*m,r_code::View	*icpp_pgm_view):r_exec::Controller(m,icpp_pgm_view){

		//	Load arguments here: one float and one Boolean.
		uint16	arg_set_index=getObject()->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=getObject()->code(arg_set_index).getAtomCount();
		if(arg_count!=2){

			std::cerr<<"test_program error: expected 2 arguments, got "<<arg_count<<std::endl;
			return;
		}
		arg1=getObject()->code(arg_set_index+1).asFloat();
		arg2=getObject()->code(arg_set_index+2).asBoolean();
	}
	
	~TestController(){
	}

	void	take_input(r_exec::View	*input,r_exec::Controller	*origin=NULL){

		//	Inputs are all types of objects - salient or that have become salient depending on their view's sync member.
		//	Manual filtering may be needed instead of pattern-matching.

		//input->object->trace();
	}
};

class	CorrelatorController:
public	r_exec::Controller{
private:
	Correlator	*correlator;

	r_comp::Decompiler	decompiler;
public:
	CorrelatorController(r_exec::_Mem	*m,r_code::View	*icpp_pgm_view):r_exec::Controller(m,icpp_pgm_view){

		//	Load arguments here.
		uint16	arg_set_index=getObject()->code(ICPP_PGM_ARGS).asIndex();
		uint16	arg_count=getObject()->code(arg_set_index).getAtomCount();

		correlator=new	Correlator();

		decompiler.init(&r_exec::Metadata);
	}
	
	~CorrelatorController(){

		delete	correlator;
	}

	void	take_input(r_exec::View	*input,r_exec::Controller	*origin=NULL){

		//	Inputs are all types of objects - salient or that have become salient depending on their view's sync member.
		//	Manual filtering is needed instead of pattern-matching.
		//	Here we take all inputs until we get an episode notification.
		std::string	episode_end="episode_end";
		if(input->object->code(0).asOpcode()==r_exec::Metadata.getClass(episode_end)->atom.asOpcode()){

			correlator->get_output();
			//	TODO (eric): exploit the output: build rgroups and inject data therein.

			//decompile(0);
			
			//	For now, we do not retrain the Correlator on more episodes: we build another correlator instead.
			//	We could also implement a method (clear()) to reset the existing correlator.
			delete	correlator;
			correlator=new	Correlator();
		}else
			correlator->take_input(input);
	}

	void	decompile(uint64	time_offset){

		mem->suspend();
		r_comp::Image	*image=((r_exec::Mem<r_exec::LObject>	*)mem)->getImage();
		mem->resume();

		uint32	object_count=decompiler.decompile_references(image);

		// from here on is different, by Bas
		static void* inst;
		struct OID2string {
			r_comp::Decompiler& decompiler;
			r_code::vector<r_code::SysObject*>& objects;
			uint32& object_count;
			uint64& time_offset;

			OID2string(r_comp::Decompiler& d, r_code::vector<r_code::SysObject*>& o, uint32& c, uint64& t):
				decompiler(d), objects(o), object_count(c), time_offset(t) {inst=this;}

			static std::string wrapper(uint32 id) {
				return ((OID2string*)inst)->impl(id);
			}

			std::string impl(uint32 id) {
				for(uint16 j = 0; j < object_count; ++j)
					if(objects[j]->oid == id) {
						std::ostringstream decompiled_code;
						decompiler.decompile_object(j, &decompiled_code, time_offset);
						std::string s = decompiled_code.str();
						for(size_t found = s.find("\n"); found != std::string::npos; found = s.find("\n"))
							s.replace(found, 1, " ");
						return s;
					}
				return "";
			}
		} closure(decompiler, image->code_segment.objects, object_count, time_offset);
		char buf[33];
		std::ofstream file((std::string("_DATA_") + itoa(rand(), buf, 10) + ".txt").c_str());
		if(file.is_open())
			correlator->dump(file, OID2string::wrapper);
		else
			std::cout << "ERROR" << std::endl;
		delete image;
	}
/*
	void	decompile(uint64	time_offset){

		mem->suspend();
		r_comp::Image	*image=((r_exec::Mem<r_exec::LObject>	*)mem)->getImage();
		mem->resume();

		uint32	object_count=decompiler.decompile_references(image);
		std::cout<<object_count<<" objects in the image\n";
		for(uint16	i=0;i<correlator->episode.size();++i){	// episode is ordered by injection times.

			for(uint16	j=0;j<object_count;++j){

				if(((SysObject	*)image->code_segment.objects[j])->oid==correlator->episode[i]){

					std::ostringstream	decompiled_code;
					decompiler.decompile_object(j,&decompiled_code,time_offset);
					std::cout<<"\n\nObject "<<correlator->episode[i]<<":\n"<<decompiled_code.str()<<std::endl;
				}
			}
		}

		delete	image;
	}
*/
};

r_exec::Controller	*test_program(r_exec::_Mem	*mem,r_code::View	*view){

	return	new	TestController(mem,view);
}

r_exec::Controller	*correlator(r_exec::_Mem	*mem,r_code::View	*view){

	return	new	CorrelatorController(mem,view);
}

////////////////////////////////////////////////////////////////////////////////

uint16	GetProgramCount(){

	return	2;
}

void	GetProgramName(char	*pgm_name){

	static	uint16	pgm_index=0;

	if(pgm_index==0){

		std::string	s="test_program";
		memcpy(pgm_name,s.c_str(),s.length());
		++pgm_index;
		return;
	}

	if(pgm_index==1){

		std::string	s="correlator";
		memcpy(pgm_name,s.c_str(),s.length());
		++pgm_index;
		return;
	}
}

