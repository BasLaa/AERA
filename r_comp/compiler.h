#ifndef	compiler_h
#define	compiler_h

#include	<fstream>
#include	<sstream>

#include	"out_stream.h"
#include	"segments.h"


using	namespace	r_code;

namespace	r_comp{

	class	dll_export	Compiler{
	private:
		
		std::istream	*in_stream;
		r_code::Image	*image;
		std::string		*error;

		Class		current_class;			//	the sys-class currently parsed
		ImageObject	*current_object;		//	the sys-object currently parsed
		uint32		current_object_index;	//	ordinal of the current sys-object in the code segment
		int32		current_view_index;		//	ordinal of a view in the sys-object's view set

		r_comp::Image	*_image;

		class	State{
		public:
			State():indents(0),
					right_indents_ahead(0),
					left_indents_ahead(0),
					pattern_lvl(0),
					no_arity_check(false){}
			State(Compiler	*c):indents(c->state.indents),
								right_indents_ahead(c->state.right_indents_ahead),
								left_indents_ahead(c->state.left_indents_ahead),
								pattern_lvl(c->state.pattern_lvl),
								no_arity_check(c->state.no_arity_check),
								stream_ptr(c->in_stream->tellg()){}
			uint16			indents;				//	1 indent = 3 char
			uint16			right_indents_ahead;	//	parsing right indents may unveil more 3-char groups than needed: that's some indents ahead. Avoids requiring a newline for each indent
			uint16			left_indents_ahead;		//	as above
			uint16			pattern_lvl;			//	add one when parsing skel in (ptn skel guards), sub one when done
			bool			no_arity_check;			//	set to true when a tail wildcard is encountered while parsing an expression, set back to false when done parsing the expression
			std::streampos	stream_ptr;
		};

		State	state;
		State	save_state();			//	called before trying to read an expression
		void	restore_state(State	s);	//	called after failing to read an expression

		void	set_error(std::string	s);

		UNORDERED_MAP<std::string,Reference>	local_references;	//	labels and variables declared inside objects (cleared before parsing each sys-object): translate to value pointers
		UNORDERED_MAP<std::string,Reference>	global_references;	//	labels declared outside sys-objects. translate to reference pointers
		bool	getReferenceIndex(const	std::string	reference_name,const	ReturnType	t,ImageObject	*object,uint16	&index,Class	*&_class);	//	index points to the reference set
																																					//	return false when not found

		//	utility
		bool	read_nil(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_set(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_nb(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_us(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_forever_us(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_nid(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_did(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_fid(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_bl(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_st(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_variable(uint16	write_index,uint16	&extent_index,bool	write,const	Class	p);
		bool	read_reference(uint16	write_index,uint16	&extent_index,bool	write,const	ReturnType	t);
		bool	read_wildcard(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_tail_wildcard(uint16	write_index,uint16	&extent_index,bool	write);

		bool	err;	//	set to true when parsing fails in the functions below
		//	all functions below return false (a) upon eof or, (b) when the class structure is not matched; in both cases, characters are pushed back
		//	sub-lexical units
		bool	comment();															//	pull comments out of the stream
		bool	separator(bool	pushback);											//	blank space or indent
		bool	right_indent(bool	pushback);										//	newline + 3 blank spaces wrt indents.top()
		bool	left_indent(bool	pushback);										//	newline - 3 blank spaces wrt indents.top()
		bool	indent(bool	pushback);												//	newline + same number of 3 blank spaces as given by indents.top()
		bool	expression_begin(bool	&indented);									//	( or right_indent
		bool	expression_end(bool	indented);										//	) or left_indent
		bool	set_begin(bool	&indented);											//	[ or []+right_indent
		bool	set_end(bool	indented);											//	] or left_indent
		bool	symbol_expr(std::string	&s);										//	finds any symbol s; detects trailing blanks, newline and )
		bool	symbol_expr_set(std::string	&s);									//	finds any symbol s; detects trailing blanks, newline, ) and ]
		bool	match_symbol_separator(const	char	*symbol,bool	pushback);	//	matches a symbol followed by a separator/left/right indent; separator/left/right indent is pushed back
		bool	match_symbol(const	char	*symbol,bool	pushback);				//	matches a symbol regardless of what follows
		bool	member(std::string	&s);											//	finds a string possibly followed by ., blanks, newline, ) and ]

		//	lexical units
		bool	nil();
		bool	nil_nb();
		bool	nil_us();
		bool	forever();
		bool	nil_nid();
		bool	nil_did();
		bool	nil_fid();
		bool	nil_bl();
		bool	nil_st();
		bool	label(std::string	&l);
		bool	variable(std::string	&v);
		bool	this_();
		bool	self();
		bool	stdin_();
		bool	stdout_();
		bool	root();
		bool	local_reference(uint16	&index,const	ReturnType	t);				//	must conform to t; indicates if the ref is to ba valuated in the value array (in_pattern set to true)
		bool	global_reference(uint16	&index,const	ReturnType	t);				//	no conformance: return type==ANY
		bool	this_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: this.res
		bool	local_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: p.res where p is a label/variable declared within the object
		bool	global_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: p.res where p is a label/variable declared outside the object
		bool	wildcard();
		bool	tail_wildcard();
		bool	timestamp(uint64	&ts);
		bool	str(std::string	&s);
		bool	number(float32	&n);
		bool	hex(uint32	&h);
		bool	boolean(bool	&b);
		bool	object(Class	&p);					//	looks first in sys_objects, then in objects
		bool	object(const	Class	&p);			//	must conform to p
		bool	sys_object(Class	&p);				//	looks only in sys_objects
		bool	sys_object(const	Class	&p);		//	must conform to p
		bool	marker(Class	&p);
		bool	op(Class	&p,const	ReturnType	t);	//	operator; must conform to t
		bool	op(const	Class	&p);				//	must conform to p
		bool	function(Class	&p);					//	device function
		bool	expression_head(Class	&p,const	ReturnType	t);															//	starts from the first element; arity does not count the head; must conform to t
		bool	expression_head(const	Class	&p);																		//	starts from the first element; arity does not count the head; must conform to p
		bool	expression_tail(bool	indented,const	Class	&p,uint16	write_index,uint16	&extent_index,bool	write);	//	starts from the second element; must conform to p
		
		//	structural units; check for heading labels
		bool	expression(bool	&indented,const	ReturnType	t,uint16	write_index,uint16	&extent_index,bool	write);	//	must conform to t
		bool	expression(bool	&indented,const	Class	&p,uint16	write_index,uint16	&extent_index,bool	write);		//	must conform to p
		bool	set(bool	&indented,uint16	write_index,uint16	&extent_index,bool	write);							//	no conformance, i.e. set of anything
		bool	set(bool	&indented,const	Class	&p,uint16	write_index,uint16	&extent_index,bool	write);			//	must conform to p
		
		uint16	set_element_count(bool	indented);	//	returns the number of elements in a set; parses the stream (write set to false) until it finds the end of the set and rewinds (write set back to true)
		bool	read(const	StructureMember	&m,bool	&indented,bool	enforce,uint16	write_index,uint16	&extent_index,bool	write);

		OutStream	*out_stream;

		bool	compile();	//	compiles one object; return false when there is an error
	public:
		Compiler();
		~Compiler();

		bool	compile(std::istream	*stream,	//	stream must be open
						r_comp::Image	*_image,
						r_code::Image	*&image,	//	image is allocated by compile()
						std::string		*error);	//	set when compile() fails, e.g. returns false

		//	Read functions for defining structure members
		//	always try to read nil (typed), a variable, a wildcrad or a tail wildcard first; then try to read the lexical unit; then try to read an expression returning the appropriate type
		//	indented: flag indicating if an indent has been found, meaning that a matching indent will have to be enforced
		//	enforce: set to true when the stream content has to conform with the type xxx in read_xxx
		//	_class: specifies the elements that shall compose a structure (expression or set)
		//	write_index: the index where the r-atom shall be written (atomic data), or where an internal pointer to a structure shall be written (structural data)
		//	extent_index: the index where to write data belonging to a structure (the internal pointer is written at write_index)
		//	write: when false, no writing in code->data is performed (needed by set_element_count())
		bool	read_any(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);		//	calls all of the functions below
		bool	read_number(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_timestamp(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_boolean(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_string(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_node(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_device(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_function(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_expression(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_set(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_view(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_mks(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_vws(bool	&indented,bool	enforce,const	Class	*p,uint16	write_index,uint16	&extent_index,bool	write);
	};
}


#endif