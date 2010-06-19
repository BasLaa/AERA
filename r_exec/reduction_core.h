#ifndef	reduction_core_h
#define	reduction_core_h

#include	"utils.h"
#include	"reduction_job.h"


using	namespace	core;

namespace	r_exec{

	class	_Mem;

	//	Pop a job and reduce - may create overlays from exisitng ones.
	//	Injects new productions in the mem - may create 1st level overlays.
	//	For each job:
	//		- reduce.
	//		- notify.
	//		- inject new rdx jobs if salient prods.
	//		- inject new update jobs if prod=grp.
	//		- inject new signaling jobs if prod=|pgm or prod=pgm with no inputs.
	class	r_exec_dll	ReductionCore:
	public	Thread{
	private:
		_Mem	*mem;
	public:
		static	thread_ret thread_function_call	Run(void	*args);
		ReductionCore(_Mem	*i);
		~ReductionCore();
	};
}


#endif