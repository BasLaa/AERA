//	usr_operators.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

#include	<iostream>
#include	<cmath>


uint16	Vec3Opcode;

////////////////////////////////////////////////////////////////////////////////

bool	add(const	r_exec::Context	&context){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.getCode()->asOpcode()==Vec3Opcode	&&	rhs.getCode()->asOpcode()==Vec3Opcode){

		context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[1].asFloat()+rhs.getCode()[1].asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[2].asFloat()+rhs.getCode()[2].asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[3].asFloat()+rhs.getCode()[3].asFloat()));
		return	true;
	}

	context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	sub(const	r_exec::Context	&context){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.getCode()->asOpcode()==Vec3Opcode	&&	rhs.getCode()->asOpcode()==Vec3Opcode){

		context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[1].asFloat()-rhs.getCode()[1].asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[2].asFloat()-rhs.getCode()[2].asFloat()));
		context.addCompoundResultPart(Atom::Float(lhs.getCode()[3].asFloat()-rhs.getCode()[3].asFloat()));
		return	true;
	}

	context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	mul(const	r_exec::Context	&context){

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.getCode()->isFloat()){

		if(rhs.getCode()->asOpcode()==Vec3Opcode){

			context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()->asFloat()*rhs.getCode()[1].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()->asFloat()*rhs.getCode()[2].asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()->asFloat()*rhs.getCode()[3].asFloat()));
			return	true;
		}
	}else	if(lhs.getCode()->asOpcode()==Vec3Opcode){

		if(rhs.getCode()->isFloat()){

			context.setCompoundResultHead(Atom::Object(Vec3Opcode,3));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()[1].asFloat()-rhs.getCode()->asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()[2].asFloat()-rhs.getCode()->asFloat()));
			context.addCompoundResultPart(Atom::Float(lhs.getCode()[3].asFloat()-rhs.getCode()->asFloat()));
			return	true;
		}
	}

	context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

bool	dis(const	r_exec::Context	&context){	

	r_exec::Context	lhs=context.getChild(1);
	r_exec::Context	rhs=context.getChild(2);

	if(lhs.getCode()->asOpcode()==Vec3Opcode	&&	rhs.getCode()->asOpcode()==Vec3Opcode){

		float32	norm2=lhs.getCode()[1].asFloat()*lhs.getCode()[1].asFloat()+rhs.getCode()[1].asFloat()*rhs.getCode()[1].asFloat()+
						lhs.getCode()[2].asFloat()*lhs.getCode()[2].asFloat()+rhs.getCode()[2].asFloat()*rhs.getCode()[2].asFloat()+
						lhs.getCode()[3].asFloat()*lhs.getCode()[3].asFloat()+rhs.getCode()[3].asFloat()*rhs.getCode()[3].asFloat();
		context.setAtomicResult(Atom::Float(sqrt(norm2)));
		return	true;
	}

	context.setAtomicResult(Atom::Nil());
	return	false;
}

////////////////////////////////////////////////////////////////////////////////

void	Init(UNORDERED_MAP<std::string,uint16>	&opcodes){

	Vec3Opcode=opcodes.find("vec3")->second;

	std::cout<<"usr operators initialized"<<std::endl;
}

uint16	GetOperatorCount(){

	return	4;
}

void	GetOperator(Operator	&op,std::string	&op_name){

	static	uint16	op_index=0;

	if(op_index==0){

		op=add;
		op_name=std::string("add");
		++op_index;
		return;
	}

	if(op_index==1){

		op=sub;
		op_name=std::string("sub");
		++op_index;
		return;
	}

	if(op_index==2){

		op=mul;
		op_name=std::string("mul");
		++op_index;
		return;
	}

	if(op_index==3){

		op=dis;
		op_name=std::string("dis");
		++op_index;
		return;
	}
}

