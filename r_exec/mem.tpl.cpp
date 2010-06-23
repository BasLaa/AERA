#include	"init.h"
#include	"replicode_defs.h"
#include	"operator.h"
#include	"group.h"
#include	"factory.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	template<class	O,class	H,class	E>	Mem<O,H,E>::Mem(){

		object_register_sem=new	FastSemaphore(1,1);
	}

	template<class	O,class	H,class	E>	Mem<O,H,E>::~Mem(){

		delete	object_register_sem;
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	r_code::Code	*Mem<O,H,E>::buildObject(r_code::SysObject	*source){

		return	new	O(source,this);
	}

	template<class	O,class	H,class	E>	r_code::Code	*Mem<O,H,E>::buildGroup(r_code::SysObject	*source){

		return	new	Group(source);
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::deleteObject(Code	*object){

		//	erase from object_register.
		object_register_sem->acquire();
		object_register.erase((O	*)object);
		object_register_sem->release();
	}

	template<class	O,class	H,class	E>	View	*Mem<O,H,E>::find_view(Code	*object,Code	*group){

		UNORDERED_SET<View	*,View::Hash,View::Equal>::const_iterator	v;

		((O	*)object)->acq_views();

		for(v=((O	*)object)->views.begin();v!=((O	*)object)->views.end();++v)
			if((*v)->get_host()==group){

				View	*found=new	View(*v);
				((O	*)object)->rel_views();
				return	found;
			}

		((O	*)object)->rel_views();
		return	NULL;
	}

	template<class	O,class	H,class	E>	Group	*Mem<O,H,E>::get_stdin()	const{

		return	_stdin;
	}
	
	template<class	O,class	H,class	E>	Group	*Mem<O,H,E>::get_stdout()	const{

		return	_stdout;
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::load(std::vector<r_code::Code	*>	*objects){	//	NB: no cov at init time.

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore(this);
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore(this);

		//	load root
		root=(Group	*)(*objects)[0];
		//	load conveniences
		_stdin=(Group	*)(*objects)[1];
		_stdout=(Group	*)(*objects)[2];
		_self=(O	*)(*objects)[3];

		for(uint32	i=1;i<objects->size();++i){

			O	*object=(O	*)(*objects)[i];

			for(uint16	i=0;i<object->initial_views.size();++i){

				//	init hosts' member_set and object's view_map.
				View	*view=(View	*)object->initial_views[i];
				view->object=object;
				Group	*host=view->get_host();
				object->views.insert(view);

				switch(GetType(object)){
				case	ObjectType::GROUP:{
					
					host->group_views[view->getOID()]=view;

					//	init viewing_group.
					bool	viewing_c_active=host->get_c_act()>host->get_c_act_thr();
					bool	viewing_c_salient=host->get_c_sln()>host->get_c_sln_thr();
					bool	viewed_visible=view->get_act_vis()>host->get_vis_thr();
					if(viewing_c_active	&&	viewing_c_salient	&&	viewed_visible)	//	visible group in a c-salient, c-active group.
						((Group	*)object)->viewing_groups[host]=view->get_cov()==0?false:true;	//	init the group's viewing groups.
					break;
				}case	ObjectType::IPGM:
					host->ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						IPGMController	*o=new	IPGMController(view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::INPUT_LESS_IPGM:
					host->input_less_ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						IPGMController	*o=new	IPGMController(view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::ANTI_IPGM:
					host->anti_ipgm_views[view->getOID()]=view;
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						IPGMController	*o=new	IPGMController(view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
					break;
				case	ObjectType::OBJECT:
				case	ObjectType::MARKER:
					host->other_views[view->getOID()]=view;
					break;
				}
			}
			object->initial_views.as_std()->clear();

			if(GetType(object)!=ObjectType::GROUP)	//	load non-group object in regsister and io map.
				object_register.insert(object);
			else
				initial_groups.push_back((Group	*)object);	//	convenience to create initial update jobs - see start().
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::start(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		uint64	now=Now();
		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();

			if(c_active){

				UNORDERED_MAP<uint32,P<View> >::const_iterator	v;

				//	build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views_begin();v!=g->input_less_ipgm_views_end();v=g->next_input_less_ipgm_view(v)){

					TimeJob	j(new	InputLessPGMSignalingJob(v->second->controller),now+g->get_spr()*base_period);
					time_job_queue.push(j);
				}

				//	build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views_begin();v!=g->anti_ipgm_views_end();v=g->next_anti_pgm_view(v)){

					TimeJob	j(new	AntiPGMSignalingJob(v->second->controller),now+Timestamp::Get(&v->second->controller->getIPGM()->references(0)->code(PGM_TSC)));
					time_job_queue.push(j);
				}

				if(c_salient){

					//	build reduction jobs for each salient view and each active overlay.
					UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
					for(v=g->views_begin();v!=g->views_end();v=g->next_view(v)){
						
						r_code::Timestamp::Set(&v->second->code(VIEW_IJT),now);	//	init injection time for the view.
						if(v->second->get_sln()>g->get_sln_thr()){	//	salient view.

							g->newly_salient_views.push_back(v->second);
							_inject_reduction_jobs(v->second,g);
						}
					}
				}
			}

			//	inject the next update job for the group.
			TimeJob	j(new	UpdateJob(g),now+g->get_upr()*base_period);
			time_job_queue.push(j);
		}

		initial_groups.clear();
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	r_comp::Image	*Mem<O,H,E>::getImage(){

		r_comp::Image	*image=new	r_comp::Image();
		UNORDERED_SET<O	*,H,E>::iterator	i;
		for(i=object_register.begin();i!=object_register.end();++i)
			image->operator	<<(*i);
		return	image;
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::inject(O	*object,View	*view){

		view->object=object;
		injectNow(view);
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::inject(View	*view){

		uint64	now=Now();
		uint64	ijt=r_code::Timestamp::Get(&view->code(VIEW_IJT));
		if(ijt<=now)
			injectNow(view);
		else{

			TimeJob	j(new	InjectionJob(view),ijt);
			time_job_queue.push(j);
		}
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::injectNow(View	*view){

		Group	*host=view->get_host();
		if(view->object->code(0).getDescriptor()==Atom::GROUP)
			_inject_group_now(view,(Group	*)view->object,host);
		else{

			O	*object;
			if(O::RequiresPacking())	//	false if LObject, true for network-aware objects.
				view->object=O::Pack(view->object);	//	non compact form will be deleted (P<> in view) if not an instance of O; compact forms are left unchanged.
			object=(O	*)view->object;

			object_register_sem->acquire();
			UNORDERED_SET<O	*,H,E>::const_iterator	it=object_register.find(object);
			if(it!=object_register.end()){

				object_register_sem->release();
				_inject_existing_object_now(view,*it,host);
			}else{	//	no equivalent object already exists: we have a new view on a new object; no need to protect either the view or the object.

				host->acquire();

				object->views.insert(view);
				uint64	now=Now();
				r_code::Timestamp::Set(&view->code(VIEW_IJT),now);

				object_register.insert(object);
				object_register_sem->release();

				switch(GetType(object)){
				case	ObjectType::IPGM:{

					host->ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						for(uint32	i=0;i<host->newly_salient_views.size();++i)
							o->take_input(host->newly_salient_views[i],this);	//	view will be copied.
					}
					break;
				}case	ObjectType::ANTI_IPGM:{
					host->anti_ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						for(uint32	i=0;i<host->newly_salient_views.size();++i)
							o->take_input(host->newly_salient_views[i],this);	//	view will be copied.

						TimeJob	j(new	AntiPGMSignalingJob(o),now+Timestamp::Get(&o->getIPGM()->references(0)->code(PGM_TSC)));
						time_job_queue.push(j);
		
					}
					break;
				}case	ObjectType::INPUT_LESS_IPGM:{
					host->input_less_ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						TimeJob	j(new	InputLessPGMSignalingJob(o),now+host->get_spr()*base_period);
						time_job_queue.push(j);
					}
					break;
				}case	ObjectType::MARKER:	//	the marker does not exist yet: add it to the mks of its references.
					for(uint32	i=0;i<object->references_size();++i){

						((O	*)object->references(i))->acq_marker_set();
						object->references(i)->markers.push_back(object);
						((O	*)object->references(i))->rel_marker_set();
					}
				case	ObjectType::OBJECT:{
					host->other_views[view->getOID()]=view;
					//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
					UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
					for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

						if(vg->second)	//	cov==true, vieiwing group c-salient and c-active (otherwise it wouldn't be a viewing group).
							injectCopyNow(view,vg->first);
					}
					break;
				}
				}

				if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
					_inject_reduction_jobs(view,host);

				if(host->get_ntf_new()==1)	//	the view cannot be a ntf view (would use injectNotificationNow instead).
					injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	factory::MkNew(object)),false);	//	the object appears for the first time in the group: notify.

				host->release();
			}
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::injectCopyNow(View	*view,Group	*destination){

		View	*copied_view=new	View(view,destination);	//	ctrl values are morphed.
		_inject_existing_object_now(copied_view,view->object,destination);
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_inject_existing_object_now(View	*view,O	*object,Group	*host){

		view->object=object;	//	the object already exists (content-wise): have the view point to the existing one.

		object->acq_views();
		View	*existing_view=find_view(object,host);
		if(!existing_view){	//	no existing view: add the view to the group and to the object's view_map.

			host->acquire();
			switch(GetType(object)){
			case	ObjectType::IPGM:
				host->ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::ANTI_IPGM:
				host->anti_ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::INPUT_LESS_IPGM:
				host->input_less_ipgm_views[view->getOID()]=view;
				break;
			case	ObjectType::OBJECT:
			case	ObjectType::MARKER:
				host->other_views[view->getOID()]=view;
				break;
			}
			host->release();

			object->views.insert(view);
			object->rel_views();
		}else{	//	call set on the ctrl values of the existing view with the new view's ctrl values. NB: org left unchanged.

			object->rel_views();

			host->pending_operations.push_back(Group::PendingOperation(existing_view->getOID(),VIEW_RES,Group::SET,view->get_res()));
			host->pending_operations.push_back(Group::PendingOperation(existing_view->getOID(),VIEW_SLN,Group::SET,view->get_sln()));
			switch(GetType(object)){
			case	ObjectType::IPGM:
			case	ObjectType::ANTI_IPGM:
			case	ObjectType::INPUT_LESS_IPGM:
				host->pending_operations.push_back(Group::PendingOperation(existing_view->getOID(),IPGM_VIEW_ACT,Group::SET,view->get_act_vis()));
				break;
			}
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_inject_group_now(View	*view,Group	*object,Group	*host){	//	groups are always new; no cov for groups; no need to protect object.

		host->acquire();

		host->group_views[view->getOID()]=view;

		uint64	now=Now();
		r_code::Timestamp::Set(&view->code(VIEW_IJT),now);
		object->views.insert(view);

		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr()){	//	host is c-salient and view is salient.

			if(view->get_act_vis()>host->get_vis_thr())	//	new visible group in a c-active and c-salient host.
				host->viewing_groups[object]=view->get_cov()==0?false:true;

			_inject_reduction_jobs(view,host);
		}

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob((Group	*)object),now+((Group	*)object)->get_upr()*base_period);
		time_job_queue.push(j);

		if(host->get_ntf_new()==1)
			injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	factory::MkNew(object)),false);	//	the group appears for the first time in the group: notify.

		host->release();
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::injectNotificationNow(View	*view,bool	lock){	//	no notification for notifications; no registration either (object_register and object_io_map) and no cov.
													//	notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
		Group	*host=view->get_host();
		LObject	*object=(LObject	*)view->object;
		
		host->acquire();	//	TODO: only if ntf_grp!=producing grp (the latter is already protected in an update call).

		r_code::Timestamp::Set(&view->code(VIEW_IJT),Now());
		host->notification_views[view->getOID()]=view;

		object->views.insert(view);
		
		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
			_inject_reduction_jobs(view,host);

		host->release();
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::update(Group	*group){

		uint64	now=Now();

		group->acquire();

		group->newly_salient_views.clear();

		//	execute pending operations.
		for(uint32	i=0;i<group->pending_operations.size();++i){

			View	*v=group->get_view(group->pending_operations[i].oid);
			if(v)
				switch(group->pending_operations[i].operation){
				case	Group::MOD:
					v->mod(group->pending_operations[i].member_index,group->pending_operations[i].value);
					break;
				case	Group::SET:
					v->set(group->pending_operations[i].member_index,group->pending_operations[i].value);
					break;
				}
		}
		group->pending_operations.clear();

		//	update group's ctrl values.
		group->update_sln_thr();	//	applies decay on sln thr. 
		group->update_act_thr();
		group->update_vis_thr();

		bool	group_was_c_active=group->get_c_act()>group->get_c_act_thr();
		bool	group_is_c_active=group->update_c_act()>group->get_c_act_thr();
		bool	group_was_c_salient=group->get_c_sln()>group->get_c_sln_thr();
		bool	group_is_c_salient=group->update_c_sln()>group->get_c_sln_thr();

		group->reset_stats();

		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=group->views_begin();v!=group->views_end();v=group->next_view(v)){

			//	update resilience.
			v->second->mod_res(-1);
			float32	res=group->update_res(v->second,this);
			if(res<=0	&&	now-v->second->get_ijt()>=group->get_upr()){	//	if now-ijt<upr, let the view live for one upr.

				//	update saliency (apply decay).
				bool	wiew_was_salient=v->second->get_sln()>group->get_sln_thr();
				float32	sln_change;
				bool	wiew_is_salient=group->update_sln(v->second,sln_change,this)>group->get_sln_thr();

				if(group_is_c_salient	&&	!wiew_was_salient	&&	wiew_is_salient)	//	record as a newly salient view.
					group->newly_salient_views.push_back(v->second);

				_initiate_sln_propagation(v->second->object,sln_change,group->get_sln_thr());	//	inject sln propagation jobs.

				if(v->second->object->code(0).getDescriptor()==Atom::GROUP){

					//	update visibility.
					bool	view_was_visible=v->second->get_act_vis()>group->get_vis_thr();
					bool	view_is_visible=v->second->update_vis()>group->get_vis_thr();
					bool	cov=v->second->get_cov()==0?false:true;

					//	update viewing groups.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: unregister as a viewing group.
							((Group	*)v->second->object)->viewing_groups.erase(group);
						else{	//	group remains c-active and c-salient.

							if(!view_was_visible){
								
								if(view_is_visible)		//	newly visible view.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}else{
								
								if(!view_is_visible)	//	the view is no longer visible.
									((Group	*)v->second->object)->viewing_groups.erase(group);
								else					//	the view is still visible, cov might have changed.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_visible)		//	update viewing groups for any visible group.
							((Group	*)v->second->object)->viewing_groups[group]=cov;
					}	
				}else	if(v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM){

					//	update activation
					bool	view_was_active=v->second->get_act_vis()>group->get_act_thr();
					bool	view_is_active=group->update_act(v->second,this)>group->get_act_thr();

					//	kill newly inactive controllers, register newly active ones.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: kill the view's controller.
							v->second->controller->kill();
						else{	//	group remains c-active and c-salient.

							if(!view_was_active){
						
								if(view_is_active)	//	register the overlay for the newly active ipgm view.
									group->new_controllers.push_back(v->second->controller);
							}else{
								
								if(!view_is_active)	//	kill the newly inactive ipgm view's overlays.
									v->second->controller->kill();
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_active)	//	register the overlay for any active ipgm view.
							group->new_controllers.push_back(v->second->controller);
					}
				}
			}else{	//	view has no resilience.

				if(v->second->object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM)	//	if ipgm view, kill the overlay.
					v->second->controller->kill();

				((O	*)v->second->object)->acq_views();
				((O	*)v->second->object)->views.erase(v->second);	//	delete view from object's views.
				((O	*)v->second->object)->rel_views();

				//	delete the view.
				if(v->second->isNotification())
					group->notification_views.erase(v->first);
				else	switch(GetType(v->second->object)){
				case	ObjectType::IPGM:
					group->ipgm_views.erase(v->first);
					break;
				case	ObjectType::ANTI_IPGM:
					group->anti_ipgm_views.erase(v->first);
					break;
				case	ObjectType::INPUT_LESS_IPGM:
					group->input_less_ipgm_views.erase(v->first);
					break;
				case	ObjectType::OBJECT:
				case	ObjectType::MARKER:
					group->other_views.erase(v->first);
					break;
				case	ObjectType::GROUP:
					group->group_views.erase(v->first);
					break;
				}
			}
		}

		if(group_is_c_salient){	//	build reduction jobs.

			//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
			UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
			for(vg=group->viewing_groups.begin();vg!=group->viewing_groups.end();++vg){

				if(vg->second)	//	cov==true.
					for(uint32	i=0;i<group->newly_salient_views.size();++i)
						if(group->newly_salient_views[i]->object->code(0).getDescriptor()!=Atom::INSTANTIATED_PROGRAM	&&	//	no cov for pgm, groups or notifications.
							group->newly_salient_views[i]->object->code(0).getDescriptor()!=Atom::GROUP					&&
							!group->newly_salient_views[i]->isNotification())
								injectCopyNow(group->newly_salient_views[i],vg->first);	//	no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
			}

			for(uint32	i=0;i<group->newly_salient_views.size();++i)
				_inject_reduction_jobs(group->newly_salient_views[i],group);
		}

		if(group_is_c_active	&&	group_is_c_salient){	//	build signaling jobs for new ipgms.

			for(uint32	i=0;i<group->new_controllers.size();++i){

				switch(GetType(group->new_controllers[i]->getIPGM())){
				case	ObjectType::ANTI_IPGM:{	//	inject signaling jobs for |ipgm (tsc).

					TimeJob	j(new	AntiPGMSignalingJob(group->new_controllers[i]),now+Timestamp::Get(&group->new_controllers[i]->getIPGM()->references(0)->code(PGM_TSC)));
					time_job_queue.push(j);
					break;
				}case	ObjectType::INPUT_LESS_IPGM:{	//	inject a signaling job for an input-less pgm (sfr).

					TimeJob	j(new	InputLessPGMSignalingJob(group->new_controllers[i]),now+group->get_spr()*base_period);
					time_job_queue.push(j);
					break;
				}
				}
			}

			group->new_controllers.clear();
		}

		group->update_stats(this);	//	triggers notifications.

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob(group),now+group->get_upr()*base_period);
		time_job_queue.push(j);

		group->release();
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::update(SaliencyPropagationJob	*j){
		
		propagate_sln((O	*)j->object,j->sln_change,j->source_sln_thr);
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::propagate_sln(O	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop!
		//	to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
		object->acq_views();
		UNORDERED_SET<View	*,View::Hash,View::Equal>::const_iterator	it;
		for(it=object->views.begin();it!=object->views.end();++it){

			float32	morphed_sln_change=View::MorphChange(change,source_sln_thr,(*it)->get_host()->get_sln_thr());
			(*it)->get_host()->pending_operations.push_back(Group::PendingOperation((*it)->getOID(),VIEW_RES,Group::MOD,morphed_sln_change));
		}
		object->rel_views();
	}

	////////////////////////////////////////////////////////////////

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_inject_reduction_jobs(View	*view,Group	*host){	//	host is assumed to be c-salient; host already protected.

		if(host->get_c_act()>host->get_c_act_thr()){	//	host is c-active.

			//	build reduction jobs from host's own inputs and own overlays.
			UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
			for(v=host->ipgm_views_with_inputs_begin();v!=host->ipgm_views_with_inputs_end();v=host->next_ipgm_view_with_inputs(v)){

				if(v->second->get_act_vis()>host->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second,this);	//	view will be copied.
			}
		}

		//	build reduction jobs from host's own inputs and overlays from viewing groups, if no cov and view is not a notification.
		//	NB: visibility is not transitive;
		//	no shadowing: if a view alresady exists in the viewing group, there will be twice the reductions: all of the identicals will be trimmed down at injection time.
		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

			if(vg->second	||	view->isNotification())	//	cov==true or notification.
				continue;

			UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
			for(v=vg->first->ipgm_views_with_inputs_begin();v!=vg->first->ipgm_views_with_inputs_end();v=vg->first->next_ipgm_view_with_inputs(v)){

				if(v->second->get_act_vis()>vg->first->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second,this);			//	view will be copied.
			}
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_initiate_sln_propagation(O	*object,float32	change,float32	source_sln_thr){
		
		if(fabs(change)>object->get_psln_thr()){

			std::vector<O	*>	path;
			path.push_back(object);

			if(GetType(object)==ObjectType::MARKER){	//	if marker, propagate to references.

				for(uint32	i=0;object->references_size();++i)
					_propagate_sln(object->references(i),change,source_sln_thr,path);
			}

			//	propagate to markers
			object->acq_marker_set();
			for(uint32	i=0;object->markers.size();++i)
				_propagate_sln(object->markers[i],change,source_sln_thr,path);
			object->rel_marker_set();
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_initiate_sln_propagation(O	*object,float32	change,float32	source_sln_thr,std::vector<O	*>	&path){
		
		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		
		if(fabs(change)>object->get_psln_thr()){

			path.push_back(object);

			if(GetType(object)==ObjectType::MARKER)	//	if marker, propagate to references.
				for(uint32	i=0;object->references_size();++i)
					_propagate_sln(object->references(i),change,source_sln_thr,path);

			//	propagate to markers
			object->acq_marker_set();
			for(uint32	i=0;object->markers.size();++i)
				_propagate_sln(object->markers[i],change,source_sln_thr,path);
			object->rel_marker_set();
		}
	}

	template<class	O,class	H,class	E>	void	Mem<O,H,E>::_propagate_sln(O	*object,float32	change,float32	source_sln_thr,std::vector<O	*>	&path){

		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		path.push_back(object);

		TimeJob	j(new	SaliencyPropagationJob(object,change,source_sln_thr),0);
		time_job_queue.push(j);
		
		_initiate_sln_propagation(object,change,source_sln_thr,path);
	}
}