//	mdl_controller.h
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

#ifndef	mdl_controller_h
#define	mdl_controller_h

#include	"hlp_overlay.h"
#include	"hlp_controller.h"
#include	"p_monitor.h"
#include	"factory.h"


namespace	r_exec{

	class	MDLOverlay:
	public	HLPOverlay{
	protected:
		MDLOverlay(Controller	*c,const	BindingMap	*bindngs);
	public:
		~MDLOverlay();

		void	load_patterns();
	};

	class	PrimaryMDLOverlay:
	public	MDLOverlay{
	protected:
		P<Code>	input;

		bool	check_simulated_chaining(BindingMap	*bm,Fact	*f_imdl,Pred	*prediction);
	public:
		PrimaryMDLOverlay(Controller	*c,const	BindingMap	*bindngs);
		~PrimaryMDLOverlay();

		Overlay	*reduce(View	*input);
	};

	class	SecondaryMDLOverlay:
	public	MDLOverlay{
	public:
		SecondaryMDLOverlay(Controller	*c,const	BindingMap	*bindngs);
		~SecondaryMDLOverlay();

		Overlay	*reduce(View	*input);
	};

	class	MDLController;
	class	Requirements{
	public:
		std::vector<P<MDLController>	>	controllers;
		P<_Fact>							f_imdl;	// f1 as in f0->pred->f1->imdl.
		bool								chaining_was_allowed;
	};
	typedef	std::pair<Requirements,Requirements>	RequirementsPair;

	// Requirements don't monitor their predictions: they don't inject any; instead, they store a f->imdl in the controlled model controllers (both primary and secondary), thus, no success injected for the productions of requirements.
	// Models controlled by requirements maintain for each prediction they make, a list of all the controllers of the requirements having allowed/inhibited said prediction.
	// P-monitors (associated to non-requirement models) propagate the outcome to the controllers associated with the prediction they monitor.
	//
	// Predictions and goals are injected in the primary group only.
	// Simulations are injected in the primary group only; no mk.rdx.
	//
	// Each time a prediction is made by a non-req model, a f->imdl is injected in both primary and secondary groups. If the input was a prediction, f->pred->f->imdl is injected instead.
	// f->imdl can also be tagged as simulations.
	//
	// Successes and failures are injected only in output groups.
	//
	// If forward chaining is inhibited (by strong reqs with better cfd than weak reqs), predictions are still produced, but not injected (no mk.rdx): this is to allow the rating of the requirements.
	class	MDLController:
	public	HLPController{
	protected:
		class	REntry:	// use for requirements.
		public	PEEntry{
		public:
			P<MDLController>	controller;	// of the requirement.
			bool				chaining_was_allowed;

			REntry(_Fact	*f_p_f_imdl,MDLController	*c,bool	chaining_was_allowed);	// f_imdl is f0 as in f0->pred->f1->imdl.

			bool	is_out_of_range(uint64	now)	const{	return	(before<now	||	after>now);	}
		};

		class	RCache{
		public:
			CriticalSection	CS;
			std::list<REntry>	positive_evidences;
			std::list<REntry>	negative_evidences;
		};

		RCache	requirements;
		RCache	simulated_requirements;

		void	_store_requirement(std::list<REntry>	*cache,REntry	&e);

		CriticalSection			p_monitorsCS;
		std::list<P<PMonitor> >	p_monitors;

		P<Code>	lhs;
		P<Code>	rhs;

		bool	_is_requirement;

		HLPController	*get_rhs_controller(bool	&strong)	const;
		float32	get_cfd()	const;

		CriticalSection	active_requirementsCS;
		UNORDERED_MAP<P<_Fact>,RequirementsPair,PHash<_Fact> >	active_requirements;	// P<_Fact>: f1 as in f0->pred->f1->imdl; requirements having allowed the production of prediction; first: wr, second: sr.		

		void	monitor_predictions(_Fact	*input);

		MDLController(r_code::View	*view);
	public:
		static	MDLController	*New(View	*view,bool	&inject_in_secondary_group);

		void	add_monitor(PMonitor	*m);
		void	remove_monitor(PMonitor	*m);

		_Fact	*get_lhs()	const;
		_Fact	*get_rhs()	const;
		Fact	*get_f_ihlp(const	BindingMap	*bindings,bool	wr_enabled)	const;

		virtual	void	store_requirement(_Fact	*f_p_f_imdl,bool	chaining_was_allowed,bool	simulation)=0;
		ChainingStatus	retrieve_imdl_fwd(BindingMap	*bm,Fact	*f_imdl,RequirementsPair	&r_p,Fact	*&ground);	// checks the requirement instances during fwd; r_p: all wrs in first, all srs in second.
		ChainingStatus	retrieve_imdl_bwd(BindingMap	*bm,Fact	*f_imdl,Fact	*&ground);	// checks the requirement instances during bwd; ground is set to the best weak requirement if chaining allowed, NULL otherwise.
		ChainingStatus	retrieve_simulated_imdl_fwd(BindingMap	*bm,Fact	*f_imdl,Controller	*root);
		ChainingStatus	retrieve_simulated_imdl_bwd(BindingMap	*bm,Fact	*f_imdl,Controller	*root);

		virtual	void	predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground)=0;
		virtual	void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures)=0;
		virtual	void	register_req_outcome(Fact	*f_pred,bool	success,bool	rate_failures){}

		bool	is_requirement()	const{	return	_is_requirement;	}
		void	register_requirement(_Fact	*f_pred,RequirementsPair	&r_p);
	};

	class	PMDLController:
	public	MDLController{
	protected:
		CriticalSection				g_monitorsCS;
		std::list<P<_GMonitor> >	g_monitors;
		std::list<P<_GMonitor> >	r_monitors;

		void	inject_goal(BindingMap	*bm,Fact	*goal,Fact	*f_imdl)	const;
		void	inject_simulation(Fact	*simulation)	const;

		void	monitor_goals(_Fact	*input);

		uint64	get_sim_thz(uint64	now,uint64 deadline)	const;

		PMDLController(r_code::View	*view);
	public:
		void	add_g_monitor(_GMonitor	*m);
		void	remove_g_monitor(_GMonitor	*m);
		void	add_r_monitor(_GMonitor	*m);
		void	remove_r_monitor(_GMonitor	*m);

		virtual	void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const=0;
				void	register_predicted_goal_outcome(Fact	*goal,BindingMap	*bm,Fact	*f_imdl,bool	success,bool	injected_goal);
		virtual	void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const=0;
	};

	// See g_monitor.h: controllers and monitors work closely together.
	//
	// Min sthz is the time allowed for simulated predictions to flow upward.
	// Max sthz sets the responsiveness of the model, i.e. limits the time waiting for simulation results, i.e. limits the latency of decision making.
	// Simulation is synchronous, i.e. is performed within the enveloppe of sthz, recursively.

	// Drives are not monitored (since they are not produced by models): they are injected periodically by user-defined pgms.
	// Drives are not observable: they cannot be predicted to succeed or fail.
	// Rhs is a drive; the model is an axiom: no rating and lives in the primary group only.
	// The model does not predict.
	// There is exactly one top-level model for each drive: hence no simulation during backward chaining.
	// Top-level models cannot have requirements.
	//
	// Backward chaining: inputs are drives.
	//	if lhs is in the fact cache, stop.
	//	if lhs is in the prediction cache, spawn a g-monitor (will be ready to catch a counter-prediction, invalidate the goal and trigger the re-issuing of a new goal).
	//	else commit to the sub-goal; this will trigger the simulation of sub-sub-goals; N.B.: commands are not simulated, commands with unbound values are not injected.
	class	TopLevelMDLController:
	public	PMDLController{
		void	abduce(BindingMap	*bm,Fact	*super_goal,float32	confidence);
		void	abduce_lhs(	BindingMap	*bm,Fact	*super_goal,_Fact	*sub_goal_target,Fact	*f_imdl,_Fact	*evidence);
	public:
		TopLevelMDLController(r_code::View	*view);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		void	store_requirement(_Fact	*f_imdl,bool	chaining_was_allowed,bool	simulation);	// never called.

		void	predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);
		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
		void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
	};

	class	SecondaryMDLController;

	// Backward chaining: inputs are goals, actual or simulated.
	// Actual goals:
	//	if lhs is in the fact cache, stop.
	//	if lhs is in the prediction cache, spawn a g-monitor (will be ready to catch a counter-prediction, invalidate the goal and re-issue a new goal).
	//	else
	//		if (before-now)*percentage<min sthz, commit sub-goal on lhs.
	//		else
	//			if chaining is allowed, simulate the lhs and spawn a g-monitor with sthz=min((before-now)*percentage,max sthz)-min sthz.
	//			else, simulate f->imdl and spawn a g-monitor with sthz=min((before-now)*percentage,max sthz)/2-min sthz.
	// Simulated goals:
	//	if lhs is in the fact cache, .
	//	if lhs is in the prediction cache,
	//	else:
	//		if sthz/2>min thz, simulate the lhs and spawn a g-monitor with sthz/2-min sthz.
	//		else predict rhs (cfd=1) and stop.
	//	Commands with unbound values are not injected.
	class	PrimaryMDLController:
	public	PMDLController{
	private:
		SecondaryMDLController	*secondary;

		void	rate_model(bool	success);

		void	abduce_lhs(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim,Fact	*ground,bool	set_before);
		void	abduce_imdl(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	abduce_simulated_lhs(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	abduce_simulated_imdl(BindingMap	*bm,Fact	*super_goal,Fact	*f_imdl,bool	opposite,float32	confidence,Sim	*sim);
		void	predict_simulated_lhs(BindingMap	*bm,bool	opposite,float32	confidence,Sim	*sim);
		void	predict_simulated_evidence(_Fact	*evidence,Sim	*sim);
	public:
		PrimaryMDLController(r_code::View	*view);

		void	set_secondary(SecondaryMDLController	*secondary);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		void	gain_activation();
		void	lose_activation();

		void	store_requirement(_Fact	*f_imdl,bool	chaining_was_allowed,bool	simulation);

		void	predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);

		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_req_outcome(_Fact	*f_imdl,bool	success,bool	rate_failures);

		void	register_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;
		void	register_simulated_goal_outcome(Fact	*goal,bool	success,_Fact	*evidence)	const;

		bool	check_imdl(Fact	*goal,BindingMap	*bm);
		bool	check_simulated_imdl(Fact	*goal,BindingMap	*bm,Controller	*root);
		void	abduce(BindingMap	*bm,Fact	*super_goal,bool	opposite,float32	confidence);
	};

	// No backward chaining.
	// Rating happens only upon the success of predictions.
	// Requirements are stroed whetwehr they come from a primary or a secondary controller.
	// Positive requirements are stored into the rhs controller, both kinds (secondary or primary: the latter case is necessary for rating the model).
	class	SecondaryMDLController:
	public	MDLController{
	private:
		PrimaryMDLController	*primary;

		void	rate_model()	const;	// record successes only.
	public:
		SecondaryMDLController(r_code::View	*view);

		void	set_primary(PrimaryMDLController	*primary);

		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		void	store_requirement(_Fact	*f_imdl,bool	chaining_was_allowed,bool	simulation);

		void	predict(BindingMap	*bm,_Fact	*input,Fact	*f_imdl,bool	chaining_was_allowed,RequirementsPair	&r_p,Fact	*ground);
		void	register_pred_outcome(Fact	*f_pred,bool	success,_Fact	*evidence,float32	confidence,bool	rate_failures);
		void	register_req_outcome(_Fact	*f_pred,bool	success,bool	rate_failures);
	};
}


#endif