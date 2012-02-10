//	cst_controller.cpp
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

#include	"cst_controller.h"
#include	"mem.h"
#include	"hlp_context.h"


namespace	r_exec{

	CSTOverlay::CSTOverlay(Controller	*c,BindingMap	*bindings):HLPOverlay(c,bindings),match_deadline(0),lowest_cfd(1){
	}

	CSTOverlay::CSTOverlay(const	CSTOverlay	*original):HLPOverlay(original->controller,original->bindings){

		patterns=original->patterns;
		predictions=original->predictions;
		simulations=original->simulations;
		match_deadline=original->match_deadline;
		lowest_cfd=original->lowest_cfd;
	}

	CSTOverlay::~CSTOverlay(){
	}

	void	CSTOverlay::load_patterns(){

		Code	*object=((HLPController	*)controller)->get_unpacked_object();
		uint16	obj_set_index=object->code(CST_OBJS).asIndex();
		uint16	obj_count=object->code(obj_set_index).getAtomCount();
		for(uint16	i=1;i<=obj_count;++i){

			_Fact	*pattern=(_Fact	*)object->get_reference(object->code(obj_set_index+i).asIndex());
			patterns.push_back(pattern);
		}
	}

	bool	CSTOverlay::can_match(uint64	now)	const{

		if(match_deadline==0)
			return	true;
		return	now-_Mem::Get()->get_time_tolerance()<=match_deadline;
	}

	void	CSTOverlay::inject_production(){

		Fact	*f_icst=((CSTController	*)controller)->get_f_icst(bindings,&inputs);
		uint64	now=Now();//f_icst->get_reference(0)->trace();

		if(simulations.size()==0){	// no simulation.

			uint64	before=bindings->get_fwd_before();
			uint64	time_to_live;
			if(now>=before)
				time_to_live=1;
			else
				time_to_live=before-now;
			if(predictions.size()){

				Pred	*prediction=new	Pred(f_icst,1);
				Fact	*f_p_f_icst=new	Fact(prediction,now,now,1,1);
				UNORDERED_SET<P<_Fact>,PHash<_Fact>	>::const_iterator	pred;
				for(pred=predictions.begin();pred!=predictions.end();++pred) // add antecedents to the prediction.
					prediction->grounds.push_back(*pred);
				((HLPController	*)controller)->inject_prediction(f_p_f_icst,f_icst,lowest_cfd,time_to_live,NULL);	// inject a f->pred->icst in the primary group, no rdx.
			}else
				((CSTController	*)controller)->inject_icst(f_icst,lowest_cfd,time_to_live);	// inject f->icst in the primary and secondary groups, and in the output groups.
		}else{	// there are simulations; the production is therefore a prediction; add the simulations to the latter.

			Pred	*prediction=new	Pred(f_icst,1);
			Fact	*f_p_f_icst=new	Fact(prediction,now,now,1,1);
			UNORDERED_SET<P<Sim>,PHash<Sim>	>::const_iterator	sim;
			for(sim=simulations.begin();sim!=simulations.end();++sim)	// add simulations to the prediction.
				prediction->simulations.push_back(*sim);
			((HLPController	*)controller)->inject_prediction(f_p_f_icst,lowest_cfd);	// inject a simulated prediction in the main group.
		}
	}

	CSTOverlay	*CSTOverlay::get_offspring(BindingMap	*map,_Fact	*input,_Fact	*bound_pattern){

		CSTOverlay	*offspring=new	CSTOverlay(this);
		bindings=map;
		patterns.remove(bound_pattern);
		inputs.push_back(input);
		if(match_deadline==0)
			match_deadline=bindings->get_fwd_before();
		float32	last_cfd;
		Pred	*prediction=input->get_pred();
		if(prediction){

			last_cfd=prediction->get_target()->get_cfd();
			if(prediction->is_simulation()){

				for(uint16	i=0;i<prediction->simulations.size();++i)
					simulations.insert(prediction->simulations[i]);
			}else{

				for(uint16	i=0;i<prediction->grounds.size();++i)
					predictions.insert(prediction->grounds[i]);
			}
		}else
			last_cfd=input->get_cfd();
		
		if(lowest_cfd>last_cfd)
			lowest_cfd=last_cfd;
		
		return	offspring;
	}

	Overlay	*CSTOverlay::reduce(View	*input){
//std::cout<<std::hex<<this<<std::dec<<" "<<input->object->get_oid()<<std::endl;
//input->object->get_reference(0)->trace();
		for(uint16	i=0;i<inputs.size();++i){	// discard inputs that already matched.

			if(((_Fact	*)input->object)==inputs[i])
				return	NULL;
		}
		
		_Fact	*input_object;
		Pred	*prediction=((_Fact	*)input->object)->get_pred();
		if(prediction)
			input_object=prediction->get_target();	// input_object is f1 as in f0->pred->f1->object.
		else
			input_object=(_Fact	*)input->object;

		P<BindingMap>	bm=new	BindingMap();
		_Fact			*bound_pattern=NULL;
		std::list<P<_Fact>	>::const_iterator	p;
		for(p=patterns.begin();p!=patterns.end();++p){

			bm->load(bindings);
			bm->reset_fwd_timings(input_object);
			if(bm->match_strict(input_object,*p,MATCH_FORWARD)){

				bound_pattern=*p;
				break;
			}
		}

		if(bound_pattern){

			if(patterns.size()==1){	// last match.

				if(!code){
					
					load_code();
					P<BindingMap>	original_bindings=bindings;
					bindings=bm;
					if(evaluate_fwd_guards()){	// may update bindings.
//std::cout<<" full match\n";
						bindings=original_bindings;
						CSTOverlay	*offspring=get_offspring(bm,(_Fact	*)input->object,bound_pattern);
						invalidate();
						inject_production();	// full match.
						return	offspring;
					}else{
//std::cout<<" guards failed\n";
						delete[]	code;
						code=NULL;
						bindings=original_bindings;
						return	NULL;
					}
				}else{	// guards already evaluated.
//std::cout<<" full match\n";
					CSTOverlay	*offspring=get_offspring(bm,(_Fact	*)input->object,bound_pattern);
					invalidate();
					inject_production();	// full match.
					return	offspring;
				}
			}else{
//std::cout<<" match\n";
				return	get_offspring(bm,(_Fact	*)input->object,bound_pattern);
			}
		}else{
//std::cout<<" no match\n";
			return	NULL;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CSTController::CSTController(r_code::View	*view):HLPController(view){

		CSTOverlay	*o=new	CSTOverlay(this,bindings);	// master overlay.
		o->load_patterns();
		overlays.push_back(o);
	}

	CSTController::~CSTController(){
	}

	void	CSTController::take_input(r_exec::View	*input){

		if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
			input->object->code(0).asOpcode()!=Opcodes::AntiFact)	// discard everything but facts and |facts.
			return;	// std::cout<<"TI: "<<get_host()->get_oid()<<" > "<<input->object->get_oid()<<std::endl;
		Controller::__take_input<CSTController>(input);
	}

	void	CSTController::reduce(r_exec::View	*input){

		_Fact	*input_object=input->object;
		if(input_object->is_invalidated())
			return;

		Goal	*goal=input_object->get_goal();
		if(goal	&&	goal->is_self_goal()	&&	!goal->is_drive()){	// goal is g->f->target.

			_Fact	*goal_target=goal->get_target();	// handle only icst.
			if(goal_target->code(0).asOpcode()==Opcodes::ICst	&&	goal_target->get_reference(0)==getObject()){	// f is f->icst; produce as many sub-goals as there are patterns in the cst.

				if(!get_requirement_count()){	// models will attempt to produce the icst

					P<BindingMap>	bm=new	BindingMap(bm);
					bm->init_from_f_ihlp(goal_target);
					if(evaluate_bwd_guards(bm))	// leaves the controller constant: no need to protect; bm may be updated.
						abduce(bm,input->object);
				}
			}
		}else{
			// std::cout<<"CTRL: "<<get_host()->get_oid()<<" > "<<input->object->get_oid()<<std::endl;
			Overlay	*offspring;
			std::list<P<Overlay> >::const_iterator	o;
			reductionCS.enter();
			uint64	now=Now();
			for(o=overlays.begin();o!=overlays.end();){

				if(!((CSTOverlay	*)*o)->can_match(now))
					o=overlays.erase(o);
				else	if((*o)->is_invalidated())
					o=overlays.erase(o);
				else{

					offspring=((CSTOverlay	*)*o)->reduce(input);
					++o;
					if(offspring)
						overlays.push_front(offspring);
				}
			}
			reductionCS.leave();
		}
	}

	void	CSTController::abduce(BindingMap	*bm,Fact	*super_goal){	// super_goal is f0->g->f1->icst or f0->g->|f1->icst.

		Goal	*g=super_goal->get_goal();
		_Fact	*super_goal_target=g->get_target();
		bool	opposite=(super_goal_target->is_anti_fact());

		float32	confidence=super_goal_target->get_cfd();

		Sim		*sim=g->sim;

		Code	*cst=get_unpacked_object();
		uint16	obj_set_index=cst->code(CST_OBJS).asIndex();
		uint16	obj_count=cst->code(obj_set_index).getAtomCount();
		uint64	now=Now();
		for(uint16	i=1;i<=obj_count;++i){

			_Fact	*pattern=(_Fact	*)cst->get_reference(cst->code(obj_set_index+i).asIndex());
			_Fact	*bound_pattern=(_Fact	*)bm->bind_pattern(pattern);
			_Fact	*evidence;
			if(opposite)
				set_opposite(bound_pattern);
			switch(check_evidences(bound_pattern,evidence)){
			case	MATCH_SUCCESS_POSITIVE:	// positive evidence, no need to produce a sub-goal: skip.
				break;
			case	MATCH_SUCCESS_NEGATIVE:	// negative evidence, no need to produce a sub-goal, the super-goal will probably fail: skip.
				break;
			case	MATCH_FAILURE:	// ignore predicted evidences.
				inject_goal(bm,super_goal,bound_pattern,sim,now,confidence,get_host());	// all sub-goals share the same sim.
				break;
			}
		}
	}

	void	CSTController::inject_goal(	BindingMap	*bm,
										Fact		*super_goal,		// f0->g->f1->icst or f0->g->|f1->icst.
										_Fact		*sub_goal_target,	// f1.
										Sim			*sim,
										uint64		now,
										float32		confidence,
										Code		*group)	const{

		sub_goal_target->set_cfd(confidence);

		Goal	*sub_goal=new	Goal(sub_goal_target,super_goal->get_goal()->get_actor(),1);
		sub_goal->sim=sim;

		_Fact	*f_icst=super_goal->get_goal()->get_target();
		_Fact	*sub_goal_f=new	Fact(sub_goal,now,now,1,1);

		View	*view=new	View(true,now,confidence,1,group,group,sub_goal_f);	// SYNC_FRONT,res=1.
		_Mem::Get()->inject(view);

		if(sim->mode==SIM_ROOT){	// no rdx for SIM_OPTIONAL or SIM_MANDATORY.

			MkRdx	*mk_rdx=new	MkRdx(f_icst,super_goal,sub_goal,1,bm);
			uint16	out_group_count=get_out_group_count();
			for(uint16	i=0;i<out_group_count;++i){

				Group	*out_group=(Group	*)get_out_group(i);
				View	*view=new	NotificationView(group,out_group,mk_rdx);
				_Mem::Get()->inject_notification(view,true);
			}
		}
	}

	Fact	*CSTController::get_f_ihlp(const	BindingMap	*bindings,bool	wr_enabled)	const{

		return	bindings->build_f_ihlp(getObject(),Opcodes::ICst,false);
	}

	Fact	*CSTController::get_f_icst(const	BindingMap	*bindings,std::vector<P<_Fact> >	*inputs)	const{

		Fact	*f_icst=get_f_ihlp(bindings,false);
		((ICST	*)f_icst->get_reference(0))->components=*inputs;
		return	f_icst;
	}

	void	CSTController::inject_icst(Fact	*production,float32	confidence,uint64	time_to_live)	const{	// production: f->icst.
		
		uint64	now=Now();
		
		Group	*primary_host=get_host();
		float32	sln_thr=primary_host->code(GRP_SLN_THR).asFloat();
		if(confidence>sln_thr){
		
			View	*view=new	View(true,now,1,Utils::GetResilience(time_to_live,primary_host->get_upr()),primary_host,primary_host,production);
			_Mem::Get()->inject(view);	// inject f->icst in the primary group: needed for hlps like M[icst -> X] and S[icst X Y].

			uint16	out_group_count=get_out_group_count();
			for(uint16	i=0;i<out_group_count;++i){

				Group	*out_group=(Group	*)get_out_group(i);
				View	*view=new	NotificationView(primary_host,out_group,production);
				_Mem::Get()->inject_notification(view,true);
			}
		}
		
		sln_thr=secondary_host->code(GRP_SLN_THR).asFloat();
		if(confidence>sln_thr){

			View	*view=new	View(true,now,1,Utils::GetResilience(time_to_live,secondary_host->get_upr()),secondary_host,primary_host,production);
			_Mem::Get()->inject(view);	// inject f->icst in the secondary group: same reason as above.
		}
	}

	void	CSTController::set_secondary_host(Group	*host){

		secondary_host=host;
	}

	Group	*CSTController::get_secondary_host()	const{

		return	secondary_host;
	}
}