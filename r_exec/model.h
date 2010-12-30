//	model.h
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

#ifndef	model_h
#define	model_h

#include	"object.h"


namespace	r_exec{

	//	Model implementation for both forward and inverse models.
	//	This class keeps track of the success rate.
	//	Model performance is expressed in markers (mk.success and mk.failure); it is not to be queried from instances of this class.
	class	r_exec_dll	Model:
	public	LObject,
	public	CriticalSection{
	private:
		uint32	output_count;	//	number of outputs (predictions or goals) produced since the model has been used.
		float32	success_count;	//	number of successes weighted by a confidence value.
		float32	failure_count;	//	number of failures weighted by a confidence value.
	public:
		Model(r_code::Mem	*m=NULL);
		Model(r_code::SysObject	*source,r_code::Mem	*m);
		~Model();

		void	register_outcome(bool	measurement,float32	confidence);	//	registers an outcome and return the success rate: measurement==true means success, failure otherwise.
		float32	get_success_rate()	const;
		float32	get_failure_rate()	const;

		void	inject_opposite(Code	*fact)	const;	//	injects the negation of what was expected (fact); called by monitors' update().
	};
}


#endif