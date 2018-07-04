//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ HUMANOBS - Replicode r_Code
//_/_/
//_/_/ Eric Nivel
//_/_/ Center for Analysis and Design of Intelligent Agents
//_/_/   Reykjavik University, Menntavegur 1, 101 Reykjavik, Iceland
//_/_/   http://cadia.ru.is
//_/_/ Copyright(c)2012
//_/_/
//_/_/ This software was developed by the above copyright holder as part of 
//_/_/ the HUMANOBS EU research project, in collaboration with the 
//_/_/ following parties:
//_/_/ 
//_/_/ Autonomous Systems Laboratory
//_/_/   Technical University of Madrid, Spain
//_/_/   http://www.aslab.org/
//_/_/
//_/_/ Communicative Machines
//_/_/   Edinburgh, United Kingdom
//_/_/   http://www.cmlabs.com/
//_/_/
//_/_/ Istituto Dalle Molle di Studi sull'Intelligenza Artificiale
//_/_/   University of Lugano and SUPSI, Switzerland
//_/_/   http://www.idsia.ch/
//_/_/
//_/_/ Institute of Cognitive Sciences and Technologies
//_/_/   Consiglio Nazionale delle Ricerche, Italy
//_/_/   http://www.istc.cnr.it/
//_/_/
//_/_/ Dipartimento di Ingegneria Informatica
//_/_/   University of Palermo, Italy
//_/_/   http://roboticslab.dinfo.unipa.it/index.php/Main/HomePage
//_/_/
//_/_/
//_/_/ --- HUMANOBS Open-Source BSD License, with CADIA Clause v 1.0 ---
//_/_/
//_/_/ Redistribution and use in source and binary forms, with or without 
//_/_/ modification, is permitted provided that the following conditions 
//_/_/ are met:
//_/_/
//_/_/ - Redistributions of source code must retain the above copyright 
//_/_/ and collaboration notice, this list of conditions and the 
//_/_/ following disclaimer.
//_/_/
//_/_/ - Redistributions in binary form must reproduce the above copyright 
//_/_/ notice, this list of conditions and the following
//_/_/ disclaimer in the documentation and/or other materials provided 
//_/_/ with the distribution.
//_/_/
//_/_/ - Neither the name of its copyright holders nor the names of its 
//_/_/ contributors may be used to endorse or promote products 
//_/_/ derived from this software without specific prior written permission.
//_/_/
//_/_/ - CADIA Clause: The license granted in and to the software under this 
//_/_/ agreement is a limited-use license. The software may not be used in 
//_/_/ furtherance of: 
//_/_/ (i) intentionally causing bodily injury or severe emotional distress 
//_/_/ to any person; 
//_/_/ (ii) invading the personal privacy or violating the human rights of 
//_/_/ any person; or 
//_/_/ (iii) committing or preparing for any act of war.
//_/_/
//_/_/ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//_/_/ "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//_/_/ LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
//_/_/ A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//_/_/ OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//_/_/ SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//_/_/ LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//_/_/ DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//_/_/ THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//_/_/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//_/_/ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//_/_/
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef	r_code_list_h
#define	r_code_list_h

#include	"../../CoreLibrary/CoreLibrary/types.h"


using	namespace	core;

namespace	r_code{

	// Minimalist list implemented as a vector.
	// Possible optimization: get rid of the std::vector and manage allocation oneself.
	// Insertion not needed for now; not implemented.
	template<typename	T>	class	list{
	protected:
		static	const	int32	null=-1;

		class	cell{	// int32: to be robust WRT realloc(); this means that Ts can hold a cell index to speed up erasure.
		public:
			int32	next;
			int32	prev;
			T		data;
			cell():next(null),prev(null){}
		};

		std::vector<cell>	cells;
		
		int32	used_cells_head;
		int32	used_cells_tail;
		int32	free_cells;	// backward links unused.
		uint32	used_cell_count;
		uint32	free_cell_count;

		void	push_back_free_cell(const	T	&t){

			int32	free=free_cells;
			free_cells=cells[free_cells].next;
			--free_cell_count;
			cells[free].data=t;
			cells[free].next=null;
			cells[free].prev=used_cells_tail;
			used_cells_tail=free;
		}

		void	push_back_new_cell(const	T	&t){

			cell	c;
			c.data=t;
			c.next=null;
			c.prev=used_cells_tail;
			cells.push_back(c);
			used_cells_tail=cells.size()-1;
		}

		void	update_used_cells_tail_state(){

			if(cells[used_cells_tail].prev!=null)
				cells[cells[used_cells_tail].prev].next=used_cells_tail;
			if(used_cells_head==null)
				used_cells_head=used_cells_tail;
			++used_cell_count;
		}

		void	push_front_free_cell(const	T	&t){

			int32	free=free_cells;
			free_cells=cells[free_cells].next;
			--free_cell_count;
			cells[free].data=t;
			cells[free].next=used_cells_head;
			cells[free].prev=null;
			used_cells_head=free;
		}

		void	push_front_new_cell(const	T	&t){

			cell	c;
			c.data=t;
			c.next=used_cells_head;
			c.prev=null;
			cells.push_back(c);
			used_cells_head=cells.size()-1;
		}

		void	update_used_cells_head_state(){

			if(cells[used_cells_head].next!=null)
				cells[cells[used_cells_head].next].prev=used_cells_head;
			if(used_cells_tail==null)
				used_cells_tail=used_cells_head;
			++used_cell_count;
		}

		void	__erase(int32	c){

			if(cells[c].prev!=null)
				cells[cells[c].prev].next=cells[c].next;
			else
				used_cells_head=cells[c].next;
			if(cells[c].next!=null)
				cells[cells[c].next].prev=cells[c].prev;
			else
				used_cells_tail=cells[c].prev;
			cells[c].next=free_cells;
			free_cells=c;
			++free_cell_count;
			--used_cell_count;
		}
		int32	_erase(int32	c){

			int32	next=cells[c].next;
			__erase(c);
			return	next;
		}
	public:
		list():used_cells_head(null),used_cells_tail(null),free_cells(null),used_cell_count(0),free_cell_count(0){}
		
		uint32	size()	const{	return	used_cell_count;	}
		void	reserve(uint32	size){	cells.reserve(size);	}
		void	clear(){

			used_cells_head=used_cells_tail=free_cells=null;
			used_cell_count=free_cell_count=0;
			cells.clear();
		}
		void	push_back(const	T	&t){

			if(free_cell_count)
				push_back_free_cell(t);
			else
				push_back_new_cell(t);
			update_used_cells_tail_state();
		}
		void	push_back(const	T	&t,int32	&location){

			if(free_cell_count){

				location=free_cells;
				push_back_free_cell(t);
			}else{

				push_back_new_cell(t);
				location=used_cells_tail;
			}
			update_used_cells_tail_state();
		}
		void	push_front(const	T	&t){

			if(free_cell_count)
				push_front_free_cell(t);
			else
				push_front_new_cell(t);
			update_used_cells_head_state();
		}
		void	push_front(const	T	&t,int32	&location){

			if(free_cell_count){

				location=free_cells;
				push_front_free_cell(t);
			}else{

				push_front_new_cell(t);
				location=used_cells_tail;
			}
			update_used_cells_head_state();
		}

		class	_iterator{
		protected:
			int32	_cell;
			_iterator(int32	c):_cell(c){}
			_iterator():_cell(null){}
		public:
			bool	operator	==(const	_iterator	&i)	const{	return	_cell==i._cell;	}
			bool	operator	!=(const	_iterator	&i)	const{	return	_cell!=i._cell;	}
		};

		class	iterator;
		class	const_iterator:
		public	_iterator{
		friend	class	list;
		private:
			const	list	*_list;
			const_iterator(const	list	*l,int32	c):_iterator(c),_list(l){}
		public:
			const_iterator():_iterator(),_list(NULL){}
			const	T	&operator	*()		const{	return	_list->cells[_cell].data;	}
			const	T	*operator	->()	const{	return	&(_list->cells[_cell].data);	}
			const_iterator	&operator	++(){
				
				_cell=_list->cells[_cell].next;
				return	*this;
			}
			const_iterator	&operator	=(const	const_iterator	&i){
				
				_list=i._list;
				_cell=i._cell;
				return	*this;
			}
			const_iterator	&operator	=(const	iterator	&i){
				
				_list=i._list;
				_cell=i._cell;
				return	*this;
			}
		};

		class	iterator:
		public	_iterator{
		friend	class	list;
		friend	class	const_iterator;
		protected:
			list	*_list;
			iterator(list	*l,int32	c):_iterator(c),_list(l){}
		public:
			iterator():_iterator(),_list(NULL){}
			T	&operator	*()		const{	return	_list->cells[_cell].data;	}
			T	*operator	->()	const{	return	&(_list->cells[_cell].data);	}
			iterator	&operator	++(){
				
				_cell=_list->cells[_cell].next;
				return	*this;
			}
			iterator	&operator	=(const	iterator	&i){
				
				_list=i._list;
				_cell=i._cell;
				return	*this;
			}
		};
	private:
		static	const_iterator	end_iterator;
	public:
		iterator		begin()			{	return	iterator(this,used_cells_head);	}
		const_iterator	begin()	const	{	return	const_iterator(this,used_cells_head);	}
		const_iterator	&end()	const	{	return	end_iterator;	}
		iterator		erase(iterator	&i){	return	iterator(this,_erase(i._cell));	}	// no check for i._cell==null.
		const_iterator	erase(const_iterator	&i){	return	const_iterator(this,_erase(i._cell));	}	// no check for i._cell==null.
		void			erase(int32	c){	__erase(c);	}	// use for random object deletion.
		void			remove(const	T	&t){

			const_iterator	i(this,used_cells_head);
			for(;i!=end_iterator;++i){

				if((*i)==t){

					erase(i);
					return;
				}
			}
		}
		T	&front(){	return	cells[used_cells_head].data;	}
		T	&back(){	return	cells[used_cells_tail].data;	}
	};

	template<typename	T>	typename	list<T>::const_iterator	list<T>::end_iterator;
}


#endif
