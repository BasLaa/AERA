//	image.tpl.cpp
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

#include	<iostream>
#include	"atom.h"


namespace	r_code{

	template<class	I>	Image<I>	*Image<I>::Build(uint32	def_size,uint32	map_size,uint32	code_size,uint32	reloc_size){

		I	*image=new(def_size+map_size+code_size+reloc_size)	I(def_size,map_size,code_size,reloc_size);
		return	(Image<I>	*)image;
	}

	template<class	I>	Image<I>	*Image<I>::Read(ifstream &stream){

		uint32	def_size;
		uint32	map_size;
		uint32	code_size;
		uint32	reloc_size
		stream>>def_size;
		stream>>map_size;
		stream>>code_size;
		stream>>reloc_size;
		Image	*image=Build(def_size,map_size,code_size,reloc_size);
		for(uint32	i=0;i<getSize();++i)
			stream>>image->data()[i];
		return	image;
	}

	template<class	I>	void	Image<I>::Write(Image<I>	*image,ofstream &stream){

		stream<<image->def_size();
		stream<<image->map_size();
		stream<<image->code_size();
		stream<<image->reloc_size();
		for(uint32	i=0;i<image->getSize();++i)
			stream<<image->data()[i];
	}

	template<class	I>	Image<I>::Image():I(){
	}

	template<class	I>	Image<I>::~Image(){
	}

	template<class	I>	word32	*Image<I>::getDefSegment(){

		return	data();
	}

	template<class	I>	uint32	Image<I>::getDefSegmentSize()	const{

		return	def_size();
	}

	template<class	I>	uint32	Image<I>::getSize()	const{

		return	def_size()+map_size()+code_size()+reloc_size();
	}

	template<class	I>	uint32	Image<I>::getObjectCount()	const{

		return	map_size();
	}

	template<class	I>	word32	*Image<I>::getObject(uint32	i){

		return	data()+data()[def_size()+i];
	}

	template<class	I>	word32	*Image<I>::getCodeSegment(){
	
		return	data()+def_size()+map_size();
	}

	template<class	I>	uint32	Image<I>::getCodeSegmentSize()	const{

		return	code_size();
	}

	template<class	I>	word32	*Image<I>::getRelocSegment(){

		return	data()+def_size()+map_size()+code_size();
	}

	template<class	I>	uint32	Image<I>::getRelocSegmentSize()	const{

		return	reloc_size();
	}

	template<class	I>	word32	*Image<I>::getRelocEntry(uint32	index){

		word32	*reloc_segment=getRelocSegment();
		for(uint32	i=0;i<getRelocSegmentSize();++i){

			if(i==index)
				return	reloc_segment+i;	//	points to the first word of the entry, i.e. the index of the object/command in the code segment
			i+=1+reloc_segment[i];			//	points to the next entry
		}

		return	NULL;
	}

	template<class	I>	void	Image<I>::trace()	const{

		std::cout<<"---Image---\n";
		std::cout<<"Size: "<<getSize()<<std::endl;
		std::cout<<"Definition Segment Size: "<<def_size()<<std::endl;
		std::cout<<"Object Map Size: "<<map_size()<<std::endl;
		std::cout<<"Code Segment Size: "<<code_size()<<std::endl;
		std::cout<<"Relocation Segment Size: "<<reloc_size()<<std::endl;

		uint32	i=0;

		std::cout<<"===Definition Segment==="<<std::endl;
		for(;i<def_size();++i)
			std::cout<<i<<" "<<data()[i]<<std::endl;

		std::cout<<"===Object Map==="<<std::endl;
		for(;i<def_size()+map_size();++i)
			std::cout<<i<<" "<<data()[i]<<std::endl;

		//	at this point, i is at the first word32 of the first object in the code segment
		std::cout<<"===Code Segment==="<<std::endl;
		uint32	code_start=def_size()+map_size();
		for(uint32	j=def_size();j<code_start;++j){	//	read object map: data[data[j]] is the first word32 of an object, data[data[j]+4] is the first atom

			uint32	object_code_size=data()[data()[j]];
			uint32	object_reference_set_size=data()[data()[j]+1];
			uint32	object_marker_set_size=data()[data()[j]+2];
			uint32	object_view_set_size=data()[data()[j]+3];
			std::cout<<"---object---\n";
			std::cout<<i++<<" code size: "<<object_reference_set_size<<std::endl;
			std::cout<<i++<<" reference set size: "<<object_reference_set_size<<std::endl;
			std::cout<<i++<<" marker set size: "<<object_marker_set_size<<std::endl;
			std::cout<<i++<<" view set size: "<<object_view_set_size<<std::endl;
			
			std::cout<<"---code---\n";
			for(;i<data()[j]+4+object_code_size;++i){

				std::cout<<i<<" ";
				((Atom	*)(data()+i))->trace();
				std::cout<<std::endl;
			}

			std::cout<<"---reference set---\n";
			for(;i<data()[j]+4+object_code_size+object_reference_set_size;++i)
				std::cout<<i<<" "<<data()[i]<<std::endl;

			std::cout<<"---marker set---\n";
			for(;i<data()[j]+4+object_code_size+object_reference_set_size+object_marker_set_size;++i)
				std::cout<<i<<" "<<data()[i]<<std::endl;

			std::cout<<"---view set---\n";
			for(uint32	k=0;k<object_view_set_size;++k){

				uint32	view_code_size=data()[i];
				uint32	view_reference_set_size=data()[i+1];

				std::cout<<"view["<<k<<"]\n";
				std::cout<<i++<<" code size: "<<view_code_size<<std::endl;
				std::cout<<i++<<" reference set size: "<<view_reference_set_size<<std::endl;

				std::cout<<"---code---\n";
				uint32	l;
				for(l=0;l<view_code_size;++i,++l){

					std::cout<<i<<" ";
					((Atom	*)(data()+i))->trace();
					std::cout<<std::endl;
				}

				std::cout<<"---reference set---\n";
				for(l=0;l<view_reference_set_size;++i,++l)
					std::cout<<i<<" "<<data()[i]<<std::endl;
			}
		}

		std::cout<<"===Relocation Segment==="<<std::endl;
		uint32	entry_count=data()[i];
		std::cout<<i++<<" entries count: "<<entry_count<<std::endl;
		for(uint32	j=0;j<entry_count;++j){

			std::cout<<"entry["<<j<<"]\n";
			uint32	reference_count=data()[i++];
			std::cout<<i<<" count: "<<reference_count<<std::endl;
			for(uint32	k=0;k<reference_count;++k){

				std::cout<<i<<" object: "<<data()[i++]<<std::endl;
				std::cout<<i<<" view: "<<data()[i++]<<std::endl;
				std::cout<<i<<" pointer: "<<data()[i++]<<std::endl;
			}
		}
	}
}