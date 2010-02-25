#ifndef	structure_member_h
#define	structure_member_h

#include	<string>

#include	"../r_code/image.h"
#include	"../r_code/atom.h"


using	namespace	r_code;

namespace	r_comp{

	class	Class;
	class	Compiler;

	typedef	enum{
		ANY=0,
		NUMBER=1,
		TIMESTAMP=2,
		SET=3,
		BOOLEAN=4,
		STRING=5,
		NODE_ID=6,
		DEVICE_ID=7,
		FUNCTION_ID=8
	}ReturnType;

	typedef	bool	(Compiler::*_Read)(bool	&,bool,const	Class	*,uint16,uint16	&,bool);	//	reads from the stream and writes in an object

	class	DefinitionSegment;
	class	StructureMember{
	public:
		typedef	enum{
			CLASS=0,		//	iterate using the class to enumerate elements
			EXPRESSION=1,	//	iterate using the class in read_expression
			SET=2			//	iterate using the class in read_set
		}Iteration;
	private:
		_Read		_read;
		ReturnType	type;
		std::string	_class;		//	when r==read_set or read_expression, _class specifies the class of said set/expression if one is targeted in particular; otherwise _class==""
		Iteration	iteration;	//	indicates how to use the _class to read the elements of the set: as an enumeration of types, as a class of expression, or as an enumeration of types to use for reading subsets
	public:
		std::string	name;	//	unused for anything but set/object/marker classes
		StructureMember(_Read		r,			//	compiler's read function
						std::string	m,			//	member's name
						std::string	p="",		//	class name of return type if r==Compiler::read_expression or name of the structure to enumerate eleemnts if r==Compiler::read_set
						Iteration	i=CLASS);	//	specified only if r==Compiler::read_set
		Class		*get_class(DefinitionSegment	*segment)	const;
		ReturnType	get_return_type()	const;
		bool		used_as_expression()	const;
		Iteration	getIteration()	const;
		_Read		read()	const;
	};
}


#endif