#include	"segments.h"

#include	<iostream>


namespace	r_comp{

	Reference::Reference(){
	}

	Reference::Reference(const	uint16	i,const	Class	&c):index(i),_class(c){
	}

	////////////////////////////////////////////////////////////////

	DefinitionSegment::DefinitionSegment(){
	}

	Class	*DefinitionSegment::getClass(std::string	&class_name){

		UNORDERED_MAP<std::string,Class>::iterator	it=classes.find(class_name);
		if(it!=classes.end())
			return	&it->second;
		return	NULL;
	}

	Class	*DefinitionSegment::getClass(uint16	opcode){

		return	&classes_by_opcodes[opcode];
	}
	
	void	DefinitionSegment::write(word32	*data){
		//	TODO
	}
		
	void	DefinitionSegment::read(word32	*data,uint32	size){
		//	TODO
	}

	uint32	DefinitionSegment::getSize(){

		return	0;	//	TODO
	}

	////////////////////////////////////////////////////////////////

	void	ObjectMap::shift(uint32	offset){

		for(uint32	i=0;i<objects.size();++i)
			objects[i]+=offset;
	}

	void	ObjectMap::write(word32	*data){

		for(uint32	i=0;i<objects.size();++i)
			data[i]=objects[i];
	}

	void	ObjectMap::read(word32	*data,uint32	size){

		for(uint32	i=0;i<size;++i)
			objects.push_back(data[i]);
	}

	uint32	ObjectMap::getSize()	const{

		return	objects.size();
	}

	////////////////////////////////////////////////////////////////

	CodeSegment::~CodeSegment(){

		for(uint32	i=0;i<objects.size();++i)
			delete	objects[i];
	}

	void	CodeSegment::write(word32	*data){

		uint32	offset=0;
		for(uint32	i=0;i<objects.size();++i){

			objects[i]->write(data+offset);
			offset+=objects[i]->getSize();
		}
	}

	void	CodeSegment::read(word32	*data,uint32	object_count){

		uint32	offset=0;
		for(uint32	i=0;i<object_count;++i){

			SysObject	*o=new	SysObject();
			o->read(data+offset);
			objects.push_back(o);
			offset+=o->getSize();
		}
	}

	uint32	CodeSegment::getSize(){

		uint32	size=0;
		for(uint32	i=0;i<objects.size();++i)
			size+=objects[i]->getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	RelocationSegment::PointerIndex::PointerIndex():object_index(0),pointer_index(0){
	}

	RelocationSegment::PointerIndex::PointerIndex(uint32	object_index,int32	view_index,uint32	pointer_index):object_index(object_index),view_index(view_index),pointer_index(pointer_index){
	}

	void	RelocationSegment::Entry::write(word32	*data){

		data[0]=pointer_indexes.size();
		for(uint32	i=0;i<pointer_indexes.size();++i){

			data[1+i*3]=pointer_indexes[i].object_index;
			data[2+i*3]=pointer_indexes[i].view_index;
			data[3+i*3]=pointer_indexes[i].pointer_index;
		}
	}

	void	RelocationSegment::Entry::read(word32	*data){

		for(uint32	i=0;i<(uint32)data[0];++i)
			pointer_indexes.push_back(PointerIndex(data[1+i*3],data[2+i*3],data[3+i*3]));
	}

	uint32	RelocationSegment::Entry::getSize()	const{

		return	1+pointer_indexes.size()*3;
	}

	void	RelocationSegment::addObjectReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-1,reference_pointer_index));
	}

	void	RelocationSegment::addViewReference(uint32	referenced_object_index,uint32	referencing_object_index,int32	referencing_view_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,referencing_view_index,reference_pointer_index));
	}

	void	RelocationSegment::addMarkerReference(uint32	referenced_object_index,uint32	referencing_object_index,uint32	reference_pointer_index){

		entries[referenced_object_index].pointer_indexes.push_back(PointerIndex(referencing_object_index,-2,reference_pointer_index));
	}
	
	void	RelocationSegment::write(word32	*data){

		data[0]=entries.size();
		uint32	offset=0;
		for(uint32	i=0;i<entries.size();++i){

			entries[i].write(data+1+offset);
			offset+=entries[i].getSize();
		}
	}

	void	RelocationSegment::read(word32	*data){

		uint32	entry_count=data[0];
		uint32	offset=0;
		for(uint32	i=0;i<entry_count;++i){

			Entry	e;
			e.read(data+offset);
			entries.push_back(e);
			offset+=e.getSize();
		}
	}

	uint32	RelocationSegment::getSize(){

		uint32	size=1;
		for(uint32	i=0;i<entries.size();++i)
			size+=entries[i].getSize();
		return	size;
	}

	////////////////////////////////////////////////////////////////

	Image::Image():map_offset(0){
	}

	Image::Image(DefinitionSegment	*definition_segment):map_offset(0),definition_segment(definition_segment){
	}

	Image	&Image::operator	>>(r_code::Image *image){

		image->def_size=definition_segment->getSize();
		image->map_size=object_map.getSize();
		image->code_size=code_segment.getSize();
		image->reloc_size=relocation_segment.getSize();

		image->data=new	word32[image->def_size+image->map_size+image->code_size+image->reloc_size];

		object_map.shift(image->def_size+image->map_size);

		definition_segment->write(image->data);
		object_map.write(image->data+image->def_size);
		code_segment.write(image->data+image->def_size+image->map_size);
		relocation_segment.write(image->data+image->def_size+image->map_size+image->code_size);

		return	*this;
	}

	Image	&Image::operator	<<(r_code::Image *image){

		definition_segment->read(image->data,image->def_size);
		object_map.read(image->data+image->def_size,image->map_size);
		code_segment.read(image->data+image->def_size+image->map_size,image->map_size);
		relocation_segment.read(image->data+image->def_size+image->map_size+image->code_size);

		return	*this;
	}

	void	Image::addObject(SysObject	*object){

		code_segment.objects.push_back(object);
		object_map.objects.push_back(map_offset);
		map_offset+=object->getSize();
	}

	Image	&Image::operator	>>(r_code::vector<Object	*>	&ram_objects){

		uint32	i;
		for(i=0;i<code_segment.objects.size();++i)
			ram_objects[i]=new	r_code::Object(code_segment.objects[i]);
		//	Translate indices into pointers
		for(i=0;i<relocation_segment.entries.size();++i){	//	for each allocated object, write its address in the reference set of the objects or views that reference it

			r_code::Object	*referenced_object=ram_objects[i];
			RelocationSegment::Entry	e=relocation_segment.entries[i];
			for(uint32	j=0;j<e.pointer_indexes.size();++j){

				RelocationSegment::PointerIndex	p=e.pointer_indexes[j];
				r_code::Object	*referencing_object=ram_objects[p.object_index];
				if(p.view_index==-1)
					referencing_object->reference_set[p.pointer_index]=referenced_object;
				else
					referencing_object->view_set[p.view_index]->reference_set[p.pointer_index]=referenced_object;
			}
			//	TBD: put marker addresses in each object's marker set (marker sets are subsets of reference sets)
			//	TBD: no need to do the same for members in groups: the Mem shall do it itself
		}
		return	*this;
	}

	Image	&Image::operator	<<(r_code::vector<Object	*>	&ram_objects){

		uint32	i;
		for(i=0;i<ram_objects.size();++i)
			*this<<ram_objects[i];
		return	*this;
	}

	Image	&Image::operator	<<(Object	*object){

		static	uint32	Last_index=0;

		UNORDERED_MAP<Object	*,uint32>::iterator	it=ptrs_to_indices.find(object);
		if(it!=ptrs_to_indices.end())
			return	*this;

		uint32	object_index;
		ptrs_to_indices[object]=object_index=Last_index++;
		SysObject	*sys_object=new	SysObject(object);
		addObject(sys_object);

		for(uint32	i=0;i<object->reference_set.size();++i)	//	follow reference pointers and recurse
			*this<<object->reference_set[i];

		buildReferences(sys_object,object,object_index);
		return	*this;
	}

	void	Image::buildReferences(SysObject	*sys_object,Object	*object,uint32	object_index){

		//	Translate pointers into indices: valuate the sys_object's references to object, incl. sys_object's view references
		uint32	i;
		uint32	referenced_object_index;
		for(i=0;i<object->reference_set.size();++i){

			referenced_object_index=ptrs_to_indices.find(object->reference_set[i])->second;
			sys_object->reference_set.push_back(referenced_object_index);
			relocation_segment.addObjectReference(referenced_object_index,object_index,i);
		}
		for(i=0;i<object->view_set.size();++i)
			for(uint32	j=0;j<object->view_set[i]->reference_set.size();++j){

				referenced_object_index=ptrs_to_indices.find(object->view_set[i]->reference_set[j])->second;
				sys_object->view_set[i]->reference_set.push_back(referenced_object_index);
				relocation_segment.addViewReference(referenced_object_index,object_index,i,j);
			}
		//	for(i=0;j<object->marker_set.size();++i)
		//		relocation_segment.addMarkerReference(ptrs_to_indices.find(object->marker_set[i])->second,object_index,i);
	}
}