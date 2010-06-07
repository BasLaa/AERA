//	segments.h
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

#ifndef	segments_h
#define	segments_h

#include	"object.h"
#include	"class.h"


using	namespace	r_code;

namespace	r_comp{

	class	Reference{
	public:
		Reference();
		Reference(const	uint16	i,const	Class	&c);
		uint16	index;
		Class	_class;
	};

	//	All classes below map components of r_code::Image into r_comp::Image.
	//	Both images are equivalent, the latter being easier to work with (uses vectors instead of a contiguous structure, that is r_code::Image::data).
	//	All read(word32*,uint32)/write(word32*) functions defined in the classes below perfom read/write operations in an r_code::Image::data.

	class	dll_export	ClassImage{
	private:
		uint32	getClassArraySize();
		uint32	getClassesSize();
		uint32	getSysClassesSize();
		uint32	getClassNamesSize();
		uint32	getOperatorNamesSize();
		uint32	getFunctionNamesSize();
	public:
		ClassImage();

		UNORDERED_MAP<std::string,Class>	classes;	//	non-sys classes, operators and device functions
		UNORDERED_MAP<std::string,Class>	sys_classes;

		r_code::vector<std::string>	class_names;		//	classes and sys-classes; does not include set classes
		r_code::vector<std::string>	operator_names;
		r_code::vector<std::string>	function_names;
		r_code::vector<Class>		classes_by_opcodes;	//	classes indexed by opcodes; used to retrieve member names; registers all classes (incl. set classes)

		Class	*getClass(std::string	&class_name);
		Class	*getClass(uint16	opcode);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize();
	};

	class	dll_export	ObjectMap{
	public:
		r_code::vector<uint32>	objects;

		void	shift(uint32	offset);

		void	write(word32	*data);
		void	read(word32		*data,uint32	size);
		uint32	getSize()	const;
	};

	class	dll_export	CodeSegment{
	public:
		r_code::vector<SysObject	*>	objects;
		
		~CodeSegment();
		
		void	write(word32	*data);
		void	read(word32		*data,uint32	object_count);
		uint32	getSize();
	};

	class	dll_export	RelocationSegment{
	public:
		class	dll_export	PointerIndex{
		public:
			uint32	object_index;	//	index of a referencing object in CodeSegment::objects
			int32	view_index;		//	index of a view in the object's view set
			uint32	pointer_index;	//	index of a pointer in CodeSegment::objects[object_index]
			PointerIndex();
			PointerIndex(uint32	object_index,int32	view_index,uint32	pointer_index);
		};
		class	dll_export	Entry{
		public:
			r_code::vector<PointerIndex>	pointer_indexes;	//	indexes of the pointers referencing the object
			void	write(word32	*data);
			void	read(word32		*data);
			uint32	getSize()	const;
		};

		r_code::vector<Entry>	entries;

		void	addObjectReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									uint32	reference_pointer_index);
		void	addViewReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									int32	referencing_view_index,
									uint32	reference_pointer_index);
		void	addMarkerReference(uint32	referenced_object_index,
									uint32	referencing_object_index,
									uint32	reference_pointer_index);

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
	};

	class	dll_export	CodeImage{
	private:
		Mem	*mem;
		uint32	map_offset;
		UNORDERED_MAP<r_code::Object	*,uint32>	ptrs_to_indices;	//	used for >> in memory
		void	buildReferences(SysObject	*sys_object,r_code::Object	*object,uint32	object_index);
	public:
		ObjectMap			object_map;
		CodeSegment			code_segment;
		RelocationSegment	relocation_segment;

		CodeImage();
		~CodeImage();

		void	bind(Mem	*m);	//	use if there is a local r_exec::Mem (as opposed to a standalone compilation application);

		void	addObject(SysObject	*object);

		void	getObjects(ClassImage	*class_image,r_code::vector<r_code::Object	*>	&ram_objects);

		CodeImage	&operator	<<	(r_code::vector<r_code::Object	*>	&ram_objects);
		CodeImage	&operator	<<	(r_code::Object	*object);

		template<class	I>	void	serialize(I	*image){

			object_map.shift(image->def_size()+image->map_size());
			object_map.write(image->data()+image->def_size());
			code_segment.write(image->data()+image->def_size()+image->map_size());
			relocation_segment.write(image->data()+image->def_size()+image->map_size()+image->code_size());
		}

		template<class	I>	void	load(I	*image){

			object_map.read(image->data()+image->def_size(),image->map_size());
			code_segment.read(image->data()+image->def_size()+image->map_size(),image->map_size());
			relocation_segment.read(image->data()+image->def_size()+image->map_size()+image->code_size());
		}
	};

	class	dll_export	Image{
	public:
		ClassImage	class_image;
		CodeImage	code_image;

		Image();
		~Image();
		
		template<class	I>	I	*serialize(){

			I	*image=(I	*)I::Build(class_image.getSize(),code_image.object_map.getSize(),code_image.code_segment.getSize(),code_image.relocation_segment.getSize());
			
			class_image.write(image->data());
			code_image.serialize(image);
			return	image;
		}

		template<class	I>	void	load(I	*image){

			class_image.read(image->data(),image->def_size());
			code_image.load(image);
		}
	};
}


#endif