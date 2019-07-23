//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ HUMANOBS - Replicode r_exec
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

#include	"mem.h"
#include	"mdl_controller.h"
#include	"model_base.h"


namespace	r_exec{

	_Mem::_Mem():r_code::Mem(),state(NOT_STARTED),deleted(false){

		new	ModelBase();
		objects.reserve(1024);
	}

	_Mem::~_Mem(){

		for(uint32	i=0;i<DebugStreamCount;++i)
			if(debug_streams[i]!=NULL)
				delete	debug_streams[i];
	}

	void	_Mem::init(	uint32	base_period,
						uint32	reduction_core_count,
						uint32	time_core_count,
						float32	mdl_inertia_sr_thr,
						uint32	mdl_inertia_cnt_thr,
						float32	tpx_dsr_thr,
						uint32	min_sim_time_horizon,
						uint32	max_sim_time_horizon,
						float32	sim_time_horizon,
						uint32	tpx_time_horizon,
						uint32	perf_sampling_period,
						float32	float_tolerance,
						uint32	time_tolerance,
						uint32	primary_thz,
						uint32	secondary_thz,
						bool	debug,
						uint32	ntf_mk_res,
						uint32	goal_pred_success_res,
						uint32	probe_level,
						uint32	traces){

		this->base_period=base_period;

		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;

		this->mdl_inertia_sr_thr=mdl_inertia_sr_thr;
		this->mdl_inertia_cnt_thr=mdl_inertia_cnt_thr;
		this->tpx_dsr_thr=tpx_dsr_thr;
		this->min_sim_time_horizon=min_sim_time_horizon;
		this->max_sim_time_horizon=max_sim_time_horizon;
		this->sim_time_horizon=sim_time_horizon;
		this->tpx_time_horizon=tpx_time_horizon;
		this->perf_sampling_period=perf_sampling_period;
		this->float_tolerance=float_tolerance;
		this->time_tolerance=time_tolerance;
		this->primary_thz=primary_thz*1000000;
		this->secondary_thz=secondary_thz*1000000;

		this->debug=debug;
		if(debug)
			this->ntf_mk_res=ntf_mk_res;
		else
			this->ntf_mk_res=1;
		this->goal_pred_success_res=goal_pred_success_res;

		this->probe_level=probe_level;

		reduction_job_count=time_job_count=0;
		reduction_job_avg_latency=_reduction_job_avg_latency=0;
		time_job_avg_latency=_time_job_avg_latency=0;

		uint32	mask=1;
		for(uint32	i=0;i<DebugStreamCount;++i){

			if(traces	&	mask)
				debug_streams[i]=NULL;
			else
				debug_streams[i]=new	NullOStream();
			mask<<=1;
		}
	}

	std::ostream	&_Mem::Output(TraceLevel	l){

		return	(_Mem::Get()->debug_streams[l]==NULL?std::cout:*(_Mem::Get()->debug_streams[l]));
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		delete	reduction_job_queue;
		delete	time_job_queue;

		delete	stop_sem;
	}

	////////////////////////////////////////////////////////////////

	Code	*_Mem::get_root()	const{

		return	_root;
	}

	Code	*_Mem::get_stdin()	const{

		return	_stdin;
	}

	Code	*_Mem::get_stdout()	const{

		return	_stdout;
	}

	Code	*_Mem::get_self()	const{

		return	_self;
	}

	////////////////////////////////////////////////////////////////

	_Mem::State	_Mem::check_state(){

		State	s;
		stateCS.enter();
		s=state;
		stateCS.leave();

		return	s;
	}

	void	_Mem::start_core(){

		core_countCS.enter();
		if(++core_count==1)
			stop_sem->acquire();
		core_countCS.leave();
	}

	void	_Mem::shutdown_core(){

		core_countCS.enter();
		if(--core_count==0)
			stop_sem->release();
		core_countCS.leave();
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::store(Code	*object){

		int32	location;
		objects.push_back(object,location);
		object->set_stroage_index(location);
	}

	bool	_Mem::load(std::vector<r_code::Code	*>	*objects,uint32	stdin_oid,	uint32	stdout_oid,uint32	self_oid){	// no cov at init time.

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore();
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore();

		Utils::SetReferenceValues(base_period,float_tolerance,time_tolerance);

		// load root (always comes first).
		_root=(Group	*)(*objects)[0];
		store((Code	*)_root);
		initial_groups.push_back(_root);

		set_last_oid(objects->size()-1);

		for(uint32	i=1;i<objects->size();++i){	// skip root as it has no initial views.

			Code	*object=(*objects)[i];
			store(object);
			
			if(object->get_oid()==stdin_oid)
				_stdin=(Group	*)(*objects)[i];
			else	if(object->get_oid()==stdout_oid)
				_stdout=(Group	*)(*objects)[i];
			else	if(object->get_oid()==self_oid)
				_self=(*objects)[i];

			switch(object->code(0).getDescriptor()){
			case	Atom::MODEL:
				unpack_hlp(object);
				//object->add_reference(NULL);	// classifier.
				ModelBase::Get()->load(object);
				break;
			case	Atom::COMPOSITE_STATE:
				unpack_hlp(object);
				break;
			case	Atom::INSTANTIATED_PROGRAM:	// refine the opcode depending on the inputs and the program type.
				if(object->get_reference(0)->code(0).asOpcode()==Opcodes::Pgm){

					if(object->get_reference(0)->code(object->get_reference(0)->code(PGM_INPUTS).asIndex()).getAtomCount()==0)
						object->code(0)=Atom::InstantiatedInputLessProgram(object->code(0).asOpcode(),object->code(0).getAtomCount());
				}else
					object->code(0)=Atom::InstantiatedAntiProgram(object->code(0).asOpcode(),object->code(0).getAtomCount());
				break;
			}

			UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	v;
			for(v=object->views.begin();v!=object->views.end();++v){

				//	init hosts' member_set.
				View	*view=(r_exec::View	*)*v;
				view->set_object(object);
				Group	*host=view->get_host();

				if(!host->load(view,object))
					return	false;
			}

			if(object->code(0).getDescriptor()==Atom::GROUP)
				initial_groups.push_back((Group	*)object);	// convenience to create initial update jobs - see start().
		}

		return	true;
	}

	void	_Mem::init_timings(uint64	now)	const{	// called at the beginning of _Mem::start(); use initial user-supplied facts' times as offsets from now.

		uint64	time_tolerance=Utils::GetTimeTolerance()*2;
		r_code::list<P<Code> >::const_iterator	o;
		for(o=objects.begin();o!=objects.end();++o){

			uint16	opcode=(*o)->code(0).asOpcode();
			if(opcode==Opcodes::Fact	||	opcode==Opcodes::AntiFact){

				uint64	after=Utils::GetTimestamp<Code>(*o,FACT_AFTER);
				uint64	before=Utils::GetTimestamp<Code>(*o,FACT_BEFORE);

				if(after<Utils::MaxTime-now)
					Utils::SetTimestamp<Code>(*o,FACT_AFTER,after+now);
				if(before<Utils::MaxTime-now-time_tolerance)
					Utils::SetTimestamp<Code>(*o,FACT_BEFORE,before+now+time_tolerance);
				else
					Utils::SetTimestamp<Code>(*o,FACT_BEFORE,Utils::MaxTime);
			}
		}
	}

	uint64	_Mem::start(){

		if(state!=STOPPED	&&	state!=NOT_STARTED)
			return	0;

		core_count=0;
		stop_sem=new	Semaphore(1,1);

		time_job_queue=new	PipeNN<P<TimeJob>,1024>();
		reduction_job_queue=new	PipeNN<P<_ReductionJob>,1024>();

		std::vector<std::pair<View	*,Group	*>	>	initial_reduction_jobs;

		uint32	i;
		uint64	now=Now();
		Utils::SetTimeReference(now);
		ModelBase::Get()->set_thz(secondary_thz);
		init_timings(now);

		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();
			
			FOR_ALL_VIEWS_BEGIN(g,v)
				Utils::SetTimestamp<View>(v->second,VIEW_IJT,now);	// init injection time for the view.
			FOR_ALL_VIEWS_END

			if(c_active){

				UNORDERED_MAP<uint32,P<View> >::const_iterator	v;

				// build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views.begin();v!=g->input_less_ipgm_views.end();++v){

					if(v->second->controller!=NULL	&&	v->second->controller->is_activated()){

						P<TimeJob>	j=new	InputLessPGMSignalingJob(v->second,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}

				// build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views.begin();v!=g->anti_ipgm_views.end();++v){

					if(v->second->controller!=NULL	&&	v->second->controller->is_activated()){

						P<TimeJob>	j=new	AntiPGMSignalingJob(v->second,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}
			}

			if(c_salient){

				// build reduction jobs for each salient view and each active overlay - regardless of the view's sync mode.
				FOR_ALL_VIEWS_BEGIN(g,v)
					
					if(v->second->get_sln()>g->get_sln_thr()){	// salient view.

						g->newly_salient_views.insert(v->second);
						initial_reduction_jobs.push_back(std::pair<View	*,Group	*>(v->second,g));
					}
				FOR_ALL_VIEWS_END
			}
			
			if(g->get_upr()>0){	// inject the next update job for the group.

				P<TimeJob>	j=new	UpdateJob(g,g->get_next_upr_time(now));
				time_job_queue->push(j);
			}
		}

		initial_groups.clear();

		state=RUNNING;

		P<TimeJob>	j=new	PerfSamplingJob(now+perf_sampling_period,perf_sampling_period);
		time_job_queue->push(j);

		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		for(uint32	i=0;i<initial_reduction_jobs.size();++i)
			initial_reduction_jobs[i].second->inject_reduction_jobs(initial_reduction_jobs[i].first);

		return	now;
	}

	void	_Mem::stop(){

		stateCS.enter();
		if(state!=RUNNING){

			stateCS.leave();
			return;
		}
		
		uint32	i;
		P<_ReductionJob>	r;
		for(i=0;i<reduction_core_count;++i)
			reduction_job_queue->push(r=new	ShutdownReductionCore());
		P<TimeJob>	t;
		for(i=0;i<time_core_count;++i)
			time_job_queue->push(t=new	ShutdownTimeCore());

		state=STOPPED;
		stateCS.leave();

		for(i=0;i<time_core_count;++i)
			Thread::Wait(time_cores[i]);

		for(i=0;i<reduction_core_count;++i)
			Thread::Wait(reduction_cores[i]);

		stop_sem->acquire();	// wait for delegates.

		reset();
	}

	////////////////////////////////////////////////////////////////

	P<_ReductionJob>	_Mem::popReductionJob(bool waitForItem){

		if(state==STOPPED)
			return	NULL;
		return reduction_job_queue->pop(waitForItem);
	}

	void	_Mem::pushReductionJob(_ReductionJob	*j){

		if(state==STOPPED)
			return;
		j->ijt=Now();
		P<_ReductionJob>	_j=j;
		reduction_job_queue->push(_j);
	}

	P<TimeJob>	_Mem::popTimeJob(bool waitForItem){

		if(state==STOPPED)
			return	NULL;
		return	time_job_queue->pop(waitForItem);
	}

	void	_Mem::pushTimeJob(TimeJob	*j){

		if(state==STOPPED)
			return;
		P<TimeJob>	_j=j;
		time_job_queue->push(_j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(Code	*command){
		// This is only for debugging
		/*
		uint16	function = (command->code(CMD_FUNCTION).atom >> 8) & 0x000000FF;
		if (function == r_exec::GetOpcode("speak")) {
			std::cout << "Speak" << std::endl;
			//command->trace();
		}
		*/
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::inject_copy(View	*view,Group	*destination){

		View	*copied_view=new	View(view,destination);	// ctrl values are morphed.
		inject_existing_object(copied_view,view->object,destination);
	}

	void	_Mem::inject_existing_object(View	*view,Code	*object,Group	*host){

		view->set_object(object);	// the object already exists (content-wise): have the view point to the existing one.
		host->inject_existing_object(view);
	}

	void	_Mem::inject_null_program(Controller	*c,Group *group,uint64	time_to_live,bool	take_past_inputs){

		uint64	now=Now();

		Code	*null_pgm=new	LObject();
		null_pgm->code(0)=Atom::NullProgram(take_past_inputs);

		uint32	res=Utils::GetResilience(now,time_to_live,group->get_upr()*Utils::GetBasePeriod());

		View	*view=new	View(View::SYNC_ONCE,now,0,res,group,NULL,null_pgm,1);
		view->controller=c;

		c->set_view(view);

		inject(view);
	}

	void	_Mem::inject_new_object(View	*view){

		Group	*host=view->get_host();
//uint64	t0,t1,t2;
		switch(view->object->code(0).getDescriptor()){
		case	Atom::GROUP:
			bind(view);

			host->inject_group(view);
			break;
		default:
//t0=Now();
			bind(view);
//t1=Now();
			host->inject_new_object(view);
//t2=Now();
//timings_report.push_back(t2-t0);
			break;
		}
	}

	void	_Mem::inject(View	*view){

		if(view->object->is_invalidated())
			return;

		Group	*host=view->get_host();

		if(host->is_invalidated())
			return;

		uint64	now=Now();
		uint64	ijt=view->get_ijt();

		if(view->object->is_registered()){	// existing object.

			if(ijt<=now)
				inject_existing_object(view,view->object,host);
			else{
				
				P<TimeJob>	j=new	EInjectionJob(view,ijt);
				time_job_queue->push(j);
			}
		}else{								// new object.

			if(ijt<=now)
				inject_new_object(view);
			else{
				
				P<TimeJob>	j=new	InjectionJob(view,ijt);
				time_job_queue->push(j);
			}
		}
	}

	void	_Mem::inject_async(View	*view){

		if(view->object->is_invalidated())
			return;

		Group	*host=view->get_host();

		if(host->is_invalidated())
			return;

		uint64	now=Now();
		uint64	ijt=view->get_ijt();

		if(ijt<=now){

			P<_ReductionJob>	j=new	AsyncInjectionJob(view);
			reduction_job_queue->push(j);
		}else{
		
			if(view->object->is_registered()){	// existing object.

				P<TimeJob>	j=new	EInjectionJob(view,ijt);
				time_job_queue->push(j);
			}else{

				P<TimeJob>	j=new	InjectionJob(view,ijt);
				time_job_queue->push(j);
			}
		}
	}

	void	_Mem::inject_hlps(std::vector<View	*>	views,Group	*destination){

		std::vector<View	*>::const_iterator	view;
		for(view=views.begin();view!=views.end();++view)
			bind(*view);

		destination->inject_hlps(views);
	}

	void	_Mem::inject_notification(View	*view,bool	lock){	// no notification for notifications; no cov.
																// notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
		Group	*host=view->get_host();

		bind(view);
		
		host->inject_notification(view,lock);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::register_reduction_job_latency(uint64	latency){

		reduction_jobCS.enter();
		++reduction_job_count;
		reduction_job_avg_latency+=latency;
		reduction_jobCS.leave();
	}
	void	_Mem::register_time_job_latency(uint64	latency){

		time_jobCS.enter();
		++time_job_count;
		time_job_avg_latency+=latency;
		time_jobCS.leave();
	}

	void	_Mem::inject_perf_stats(){

		reduction_jobCS.enter();
		time_jobCS.enter();

		int64	d_reduction_job_avg_latency;
		if(reduction_job_count>0){

			reduction_job_avg_latency/=reduction_job_count;
			d_reduction_job_avg_latency=reduction_job_avg_latency-_reduction_job_avg_latency;
		}else
			reduction_job_avg_latency=d_reduction_job_avg_latency=0;

		int64	d_time_job_avg_latency;
		if(time_job_count>0){

			time_job_avg_latency/=time_job_count;
			d_time_job_avg_latency=time_job_avg_latency-_time_job_avg_latency;
		}else
			time_job_avg_latency=d_time_job_avg_latency=0;

		Code	*perf=new	Perf(reduction_job_avg_latency,d_reduction_job_avg_latency,time_job_avg_latency,d_time_job_avg_latency);

		// reset stats.
		reduction_job_count=time_job_count=0;
		_reduction_job_avg_latency=reduction_job_avg_latency;
		_time_job_avg_latency=time_job_avg_latency;

		time_jobCS.leave();
		reduction_jobCS.leave();

		// inject f->perf in stdin.
		uint64	now=Now();
		Code	*f_perf=new	Fact(perf,now,now+perf_sampling_period,1,1);
		View	*view=new	View(View::SYNC_ONCE,now,1,1,_stdin,NULL,f_perf);	// sync front, sln=1, res=1.
		inject(view);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::propagate_sln(Code	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	loops are prevented within one call, but not accross several upr:
		//		- feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop spreading accross several upr.
		//		- to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
		object->acq_views();

		if(object->views.size()==0){

			object->invalidate();
			object->rel_views();
			return;
		}

		UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	it;
		for(it=object->views.begin();it!=object->views.end();++it){

			float32	morphed_sln_change=View::MorphChange(change,source_sln_thr,((r_exec::View*)*it)->get_host()->get_sln_thr());
			if(morphed_sln_change!=0)
				((r_exec::View*)*it)->get_host()->pending_operations.push_back(new	Group::Mod(((r_exec::View*)*it)->get_oid(),VIEW_SLN,morphed_sln_change));
		}
		object->rel_views();
	}

	//DEPRECATED///////////////////////////////////////////////////
	
	/*r_exec_dll r_exec::Mem<r_exec::LObject> *Run(const	char	*user_operator_library_path,
												uint64			(*time_base)(),
												const	char	*seed_path,
												const	char	*source_file_name)
	{
		r_exec::Init(user_operator_library_path,time_base,seed_path );

		srand(r_exec::Now());
		Random::Init();

		std::string	error;
		r_exec::Compile(source_file_name,error);

		r_exec::Mem<r_exec::LObject> *mem = new r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.get_objects(mem,ram_objects);

		mem->init(	100000,	// base period.
					3,		// reduction core count.
					1,		// time core count.
					0.9,	// mdl inertia sr thr.
					10,		// mdl inertia cnt thr.
					0.1,	// tpx_dsr_thr.
					25000,	// min_sim time horizon
					100000,	// max_sim time horizon
					0.3,	// sim time horizon
					500000,	// tpx time horizon
					250000,	// perf sampling period
					0.1,	// float tolerance.
					10000,	// time tolerance.
					3600000,// primary thz.
					7200000,// secondary thz.
					false,	// debug.
					1000,	// ntf marker resilience.
					1000,	// goal pred success resilience.
					2);		// probe level.

		uint32	stdin_oid;
		std::string	stdin_symbol("stdin");
		uint32	stdout_oid;
		std::string	stdout_symbol("stdout");
		uint32	self_oid;
		std::string	self_symbol("self");
		UNORDERED_MAP<uint32,std::string>::const_iterator	n;
		for(n=r_exec::Seed.object_names.symbols.begin();n!=r_exec::Seed.object_names.symbols.end();++n){

			if(n->second==stdin_symbol)
				stdin_oid=n->first;
			else	if(n->second==stdout_symbol)
				stdout_oid=n->first;
			else	if(n->second==self_symbol)
				self_oid=n->first;
		}

		mem->load(ram_objects.as_std(),stdin_oid,stdout_oid,self_oid);

		return mem;
	}*/
	////////////////////////////////////////////////////////////////

	void	_Mem::unpack_hlp(Code	*hlp)	const{	// produces a new object (featuring a set of pattern objects instread of a set of embedded pattern expressions) and add it as a hidden reference to the original (still packed) hlp.

		Code	*unpacked_hlp=new	LObject();	// will not be transmitted nor decompiled.

		for(uint16	i=0;i<hlp->code_size();++i)
			unpacked_hlp->code(i)=hlp->code(i);

		uint16	pattern_set_index=hlp->code(HLP_OBJS).asIndex();
		uint16	pattern_count=hlp->code(pattern_set_index).getAtomCount();
		for(uint16	i=1;i<=pattern_count;++i){	// init the new references with the facts; turn the exisitng i-ptrs into r-ptrs.

			Code	*fact=unpack_fact(hlp,hlp->code(pattern_set_index+i).asIndex());
			unpacked_hlp->add_reference(fact);
			unpacked_hlp->code(pattern_set_index+i)=Atom::RPointer(unpacked_hlp->references_size()-1);
		}

		uint16	group_set_index=hlp->code(HLP_OUT_GRPS).asIndex();
		uint16	group_count=hlp->code(group_set_index++).getAtomCount();
		for(uint16	i=0;i<group_count;++i){	// append the out_groups to the new references; adjust the exisitng r-ptrs.

			unpacked_hlp->add_reference(hlp->get_reference(hlp->code(group_set_index+i).asIndex()));
			unpacked_hlp->code(group_set_index+i)=Atom::RPointer(unpacked_hlp->references_size()-1);
		}

		uint16	invalid_point=pattern_set_index+pattern_count+1;	// index of what is after set of the patterns.
		uint16	valid_point=hlp->code(HLP_FWD_GUARDS).asIndex();	// index of the first atom that does not belong to the patterns.
		uint16	invalid_zone_length=valid_point-invalid_point;
		for(uint16	i=valid_point;i<hlp->code_size();++i){	//	shift the valid code upward; adjust i-ptrs.

			Atom	h_atom=hlp->code(i);
			switch(h_atom.getDescriptor()){
			case	Atom::I_PTR:
				unpacked_hlp->code(i-invalid_zone_length)=Atom::IPointer(h_atom.asIndex()-invalid_zone_length);
				break;
			case	Atom::ASSIGN_PTR:
				unpacked_hlp->code(i-invalid_zone_length)=Atom::AssignmentPointer(h_atom.asAssignmentIndex(),h_atom.asIndex()-invalid_zone_length);
				break;
			default:
				unpacked_hlp->code(i-invalid_zone_length)=h_atom;
				break;
			}
		}

		// adjust set indices.
		unpacked_hlp->code(HLP_FWD_GUARDS)=Atom::IPointer(hlp->code(CST_FWD_GUARDS).asIndex()-invalid_zone_length);
		unpacked_hlp->code(HLP_BWD_GUARDS)=Atom::IPointer(hlp->code(CST_BWD_GUARDS).asIndex()-invalid_zone_length);
		unpacked_hlp->code(HLP_OUT_GRPS)=Atom::IPointer(hlp->code(CST_OUT_GRPS).asIndex()-invalid_zone_length);

		uint16	unpacked_code_length=hlp->code_size()-invalid_zone_length;
		unpacked_hlp->resize_code(unpacked_code_length);
		hlp->add_reference(unpacked_hlp);
	}

	Code	*_Mem::unpack_fact(Code	*hlp,uint16	fact_index)	const{

		Code	*fact=new	LObject();
		Code	*fact_object;
		uint16	fact_size=hlp->code(fact_index).getAtomCount()+1;
		for(uint16	i=0;i<fact_size;++i){

			Atom	h_atom=hlp->code(fact_index+i);
			switch(h_atom.getDescriptor()){
			case	Atom::I_PTR:
				fact->code(i)=Atom::RPointer(fact->references_size());
				fact_object=unpack_fact_object(hlp,h_atom.asIndex());
				fact->add_reference(fact_object);
				break;
			case	Atom::R_PTR:	// case of a reference to an exisitng object.
				fact->code(i)=Atom::RPointer(fact->references_size());
				fact->add_reference(hlp->get_reference(h_atom.asIndex()));
				break;
			default:
				fact->code(i)=h_atom;
				break;
			}
		}

		return	fact;
	}

	Code	*_Mem::unpack_fact_object(Code	*hlp,uint16	fact_object_index)	const{

		Code	*fact_object=new	LObject();
		_unpack_code(hlp,fact_object_index,fact_object,fact_object_index);
		return	fact_object;
	}

	void	_Mem::_unpack_code(Code	*hlp,uint16	fact_object_index,Code	*fact_object,uint16	read_index)	const{

		Atom	h_atom=hlp->code(read_index);
		uint16	code_size=h_atom.getAtomCount()+1;
		uint16	write_index=read_index-fact_object_index;
		for(uint16	i=0;i<code_size;++i){

			switch(h_atom.getDescriptor()){
			case	Atom::R_PTR:
				fact_object->code(write_index+i)=Atom::RPointer(fact_object->references_size());
				fact_object->add_reference(hlp->get_reference(h_atom.asIndex()));
				break;
			case	Atom::I_PTR:
				fact_object->code(write_index+i)=Atom::IPointer(h_atom.asIndex()-fact_object_index);
				_unpack_code(hlp,fact_object_index,fact_object,h_atom.asIndex());
				break;
			default:
				fact_object->code(write_index+i)=h_atom;
				break;
			}

			h_atom=hlp->code(read_index+i+1);
		}
	}

	void	_Mem::pack_hlp(Code	*hlp)	const{	// produces a new object where a set of pattern objects is transformed into a packed set of pattern code.

		Code	*unpacked_hlp=clone(hlp);

		std::vector<Atom>	trailing_code;	//	copy of the original code (the latter will be overwritten by packed facts).
		uint16	trailing_code_index=hlp->code(HLP_FWD_GUARDS).asIndex();
		for(uint16	i=trailing_code_index;i<hlp->code_size();++i)
			trailing_code.push_back(hlp->code(i));

		uint16	group_set_index=hlp->code(HLP_OUT_GRPS).asIndex();
		uint16	group_count=hlp->code(group_set_index).getAtomCount();

		std::vector<P<Code>	>	references;

		uint16	pattern_set_index=hlp->code(HLP_OBJS).asIndex();
		uint16	pattern_count=hlp->code(pattern_set_index).getAtomCount();
		uint16	insertion_point=pattern_set_index+pattern_count+1;	// point from where compacted code is to be inserted.
		uint16	extent_index=insertion_point;
		for(uint16	i=0;i<pattern_count;++i){

			Code	*pattern_object=hlp->get_reference(i);
			hlp->code(pattern_set_index+i+1)=Atom::IPointer(extent_index);
			pack_fact(pattern_object,hlp,extent_index,&references);
		}

		uint16	inserted_zone_length=extent_index-insertion_point;

		for(uint16	i=0;i<trailing_code.size();++i){	// shift the trailing code downward; adjust i-ptrs.

			Atom	t_atom=trailing_code[i];
			switch(t_atom.getDescriptor()){
			case	Atom::I_PTR:
				hlp->code(i+extent_index)=Atom::IPointer(t_atom.asIndex()+inserted_zone_length);
				break;
			case	Atom::ASSIGN_PTR:
				hlp->code(i+extent_index)=Atom::AssignmentPointer(t_atom.asAssignmentIndex(),t_atom.asIndex()+inserted_zone_length);
				break;
			default:
				hlp->code(i+extent_index)=t_atom;
				break;
			}
		}

		// adjust set indices.
		hlp->code(CST_FWD_GUARDS)=Atom::IPointer(hlp->code(HLP_FWD_GUARDS).asIndex()+inserted_zone_length);
		hlp->code(CST_BWD_GUARDS)=Atom::IPointer(hlp->code(HLP_BWD_GUARDS).asIndex()+inserted_zone_length);
		hlp->code(CST_OUT_GRPS)=Atom::IPointer(hlp->code(HLP_OUT_GRPS).asIndex()+inserted_zone_length);

		group_set_index+=inserted_zone_length;
		for(uint16	i=1;i<=group_count;++i){	//	append the out_groups to the new references; adjust the exisitng r-ptrs.

			references.push_back(hlp->get_reference(hlp->code(group_set_index+i).asIndex()));
			hlp->code(group_set_index+i)=Atom::RPointer(references.size()-1);
		}

		hlp->set_references(references);

		hlp->add_reference(unpacked_hlp);	// hidden reference.
	}

	void	_Mem::pack_fact(Code	*fact,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const{

		uint16	extent_index=write_index+fact->code_size();
		for(uint16	i=0;i<fact->code_size();++i){

			Atom	p_atom=fact->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::R_PTR:	//	transform into a i_ptr and pack the pointed object.
				hlp->code(write_index)=Atom::IPointer(extent_index);
				pack_fact_object(fact->get_reference(p_atom.asIndex()),hlp,extent_index,references);
				++write_index;
				break;
			default:
				hlp->code(write_index)=p_atom;
				++write_index;
				break;
			}
		}
		write_index=extent_index;
	}

	void	_Mem::pack_fact_object(Code	*fact_object,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const{

		uint16	extent_index=write_index+fact_object->code_size();
		uint16	offset=write_index;
		for(uint16	i=0;i<fact_object->code_size();++i){

			Atom	p_atom=fact_object->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::R_PTR:{	//	append this reference to the hlp's if not already there.
				Code	*reference=fact_object->get_reference(p_atom.asIndex());
				bool	found=false;
				for(uint16	i=0;i<references->size();++i){

					if((*references)[i]==reference){

						hlp->code(write_index)=Atom::RPointer(i);
						found=true;
						break;
					}
				}
				if(!found){

					hlp->code(write_index)=Atom::RPointer(references->size());
					references->push_back(reference);
				}
				++write_index;
				break;
			}case	Atom::I_PTR:	//	offset the ptr by write_index. PB HERE.
				hlp->code(write_index)=Atom::IPointer(offset+p_atom.asIndex());
				++write_index;
				break;
			default:
				hlp->code(write_index)=p_atom;
				++write_index;
				break;
			}
		}
	}

	Code	*_Mem::clone(Code	*original)	const{	// shallow copy; oid not copied.

		Code	*_clone=build_object(original->code(0));
		uint16	opcode=original->code(0).asOpcode();
		if(opcode==Opcodes::Ont	||	opcode==Opcodes::Ent)
			return	original;

		for(uint16	i=0;i<original->code_size();++i)
			_clone->code(i)=original->code(i);
		for(uint16	i=0;i<original->references_size();++i)
			_clone->add_reference(original->get_reference(i));
		return	_clone;
	}

	////////////////////////////////////////////////////////////////

	r_comp::Image	*_Mem::get_models(){

		r_comp::Image	*image=new	r_comp::Image();
		image->timestamp=Now();

		r_code::list<P<Code> >	models;
		ModelBase::Get()->get_models(models);	// protected by ModelBase.
		image->add_objects(models);

		return	image;
	}

	////////////////////////////////////////////////////////////////

	MemStatic::MemStatic():_Mem(),last_oid(-1){
	}

	MemStatic::~MemStatic(){
	}

	void	MemStatic::bind(View	*view){

		Code	*object=view->object;
		object->views.insert(view);
		objectsCS.enter();
		object->set_oid(++last_oid);
		if(object->code(0).getDescriptor()==Atom::NULL_PROGRAM){

			objectsCS.leave();
			return;
		}
		int32	location;
		objects.push_back(object,location);
		object->set_stroage_index(location);
		objectsCS.leave();
	}
	void	MemStatic::set_last_oid(int32	oid){

		last_oid=oid;
	}

	void	MemStatic::delete_object(r_code::Code	*object){	// called only if the object is registered, i.e. has a valid storage index.

		if(deleted)
			return;

		objectsCS.enter();
		objects.erase(object->get_storage_index());
		objectsCS.leave();
	}

	r_comp::Image	*MemStatic::get_objects(){

		r_comp::Image	*image=new	r_comp::Image();
		image->timestamp=Now();
		
		objectsCS.enter();
		image->add_objects(objects);
		objectsCS.leave();

		return	image;
	}

	////////////////////////////////////////////////////////////////

	MemVolatile::MemVolatile():_Mem(),last_oid(-1){
	}

	MemVolatile::~MemVolatile(){
	}

	uint32	MemVolatile::get_oid(){

		return	Atomic::Increment32(&last_oid);
	}

	void	MemVolatile::set_last_oid(int32	oid){

		last_oid=oid;
	}

	void	MemVolatile::bind(View	*view){

		Code	*object=view->object;
		object->views.insert(view);
		object->set_oid(get_oid());
	}
}