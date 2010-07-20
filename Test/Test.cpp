//	test.cpp
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

#include	"decompiler.h"
#include	"mem.h"
#include	"init.h"
#include	"image_impl.h"


//#define	DECOMPILE_ONE_BY_ONE

using	namespace	r_comp;

void	decompile(Decompiler	&decompiler,r_comp::Image	*image){

#ifdef	DECOMPILE_ONE_BY_ONE
	uint32	object_count=decompiler.decompile_references(image);
	std::cout<<object_count<<" objects in the image\n";
	while(1){

		std::cout<<"Which object (-1 to exit)?\n";
		int32	index;std::cin>>index;
		if(index==-1)
			break;
		if(index>=object_count){

			std::cout<<"there is only "<<object_count<<" objects\n";
			continue;
		}
		std::ostringstream	decompiled_code;
		decompiler.decompile_object(index,&decompiled_code);
		std::cout<<"... done\n";
		std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	}
#else
	std::cout<<"\ndecompiling ...\n";
	std::ostringstream	decompiled_code;
	decompiler.decompile(image,&decompiled_code);
	std::cout<<"... done\n";
	std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
#endif
}

int32	main(int	argc,char	**argv){

	core::Time::Init(1000);

	std::cout<<"compiling ...\n";
	r_exec::Init("C:/Work/Replicode/Debug/usr_operators.dll",Time::Get,"C:/Work/Replicode/Test/user.classes.replicode");
	std::cout<<"... done\n";

	std::string	error;
	if(!r_exec::Compile(argv[1],error)){

		std::cerr<<" <- "<<error<<std::endl;
		return	1;
	}else{

		Decompiler	decompiler;
		decompiler.init(&r_exec::Metadata);

		r_comp::Image	*image;

		r_exec::Mem<r_exec::LObject>	*mem=new	r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.getObjects(mem,ram_objects);

		mem->init(100000,8,8,20);
		mem->load(ram_objects.as_std());
		mem->start();
		//uint32	in;std::cout<<"Enter a number to stop the rMem:\n";std::cin>>in;
		std::cout<<"\nSleeping 1000\n";
		Thread::Sleep(1000);
		mem->suspend();
		image=mem->getImage();
		decompile(decompiler,image);
		delete	image;
		mem->resume();
		/*
		Thread::Sleep(400);
		mem->suspend();
		//TimeProbe	probe;
		//probe.set();
		image=mem->getImage();
		//probe.check();
		decompile(decompiler,image);
		delete	image;
		//std::cout<<"getImage(): "<<probe.us()<<std::endl;
		mem->resume();
		*/
		Thread::Sleep(500);

		std::cout<<"\nstopping rMem\n";
		mem->stop();
		delete	mem;
	}

	return	0;
}
