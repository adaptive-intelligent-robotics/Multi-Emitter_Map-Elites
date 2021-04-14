#ifndef QD_UCB_EMITTER_POOL_HPP
#define QD_UCB_EMITTER_POOL_HPP

#include <boost/circular_buffer.hpp>

namespace sferes {
  namespace qd {
    namespace selector {

      struct UCB_init_pool
      {
	UCB_init_pool(size_t size):_size(size){}
	
	template <typename T>
	void operator()(T &subpool) const
	{
	  for(size_t i=0; i<_size; i++)
	    subpool.push_back(std::make_shared<typename T::value_type::element_type>());
	}

	size_t _size;
      };

      template<typename Phen, typename EA>
      struct UCB_form_pop
      {
	typedef boost::shared_ptr<Phen> indiv_t;
	
	UCB_form_pop(std::vector<indiv_t>& pop, const EA& ea, const std::vector<size_t>& S, size_t& it):_pop(pop), _ea(ea),_it(it), _S(S){}
	

	template <typename T>
	void operator()(T &subpool) const
	{
	  for (auto& emit : subpool)
	    {
	      if(std::find(_S.begin(), _S.end(), _it) != _S.end()){
		//emit->update_batch();
		emit->gen_batch(_ea);
		_pop.insert(_pop.end(), emit->get_batch().begin(), emit->get_batch().end());
	      }
	      _it++;
	    }

	}
	std::vector<indiv_t>& _pop;
	const EA& _ea;
	size_t& _it;
	const std::vector<size_t>& _S;
      };


      struct UCB_collect_reward
      {
	UCB_collect_reward(std::vector<std::pair<size_t,float>>& reward, const std::vector<size_t>& S, size_t& it):_reward(reward), _it(it), _S(S){
	}
	

	template <typename T>
	void operator()(T &subpool) const
	{
	  for (auto& emit : subpool)
	    {
	      if(std::find(_S.begin(), _S.end(), _it) != _S.end()){
		emit->update_batch();
		if(emit->check_termination()){
		  float val = emit->get_score();
		  _reward.push_back({_it, val});
		}
	      }
	      _it++;
	    }
	  
	}
	std::vector<std::pair<size_t, float>>& _reward;
	size_t& _it;
	const std::vector<size_t>& _S;
      };
      
      template <typename Phen, typename Params, typename ... Emitters >
      struct UCB_Emitter_pool {
	UCB_Emitter_pool():_history(50),_nb_emitters(sizeof...(Emitters)*Params::Emitter_pool::nb_emitters)
	{
	  std::cout<<"UCB"<<std::endl;
	  std::cout<<"Number of emitter types: "<<sizeof...(Emitters)<<std::endl;
	  std::cout<<"Number of emitters: "<<_nb_emitters<<std::endl;
	  
	  boost::fusion::for_each(_emitter_pool,UCB_init_pool(Params::Emitter_pool::nb_emitters));
	}
	
	typedef boost::shared_ptr<Phen> indiv_t;
	
	template <typename EA>
	void operator()(std::vector<indiv_t>& pop, const EA& ea) 
	{
	  //std::cout<<"start_ selector"<<std::endl;
	  assert(pop.size()==0 || pop.size()==ea.added().size());

	  for(size_t i=0; i <pop.size(); i++){
	    pop[i]->fit().added=std::get<0>(ea._updates[i]);
	    pop[i]->fit().new_cell=std::get<1>(ea._updates[i]);
	    pop[i]->fit().improvement=std::get<2>(ea._updates[i]);
	  }
	  std::vector<std::pair<size_t,float>> reward;
	  size_t it=0;
	  boost::fusion::for_each(_emitter_pool,UCB_collect_reward(reward,_S,it));
	  /*for(size_t i=0;i<reward.size();i++)
	    std::cout<< reward[i].first <<" "<<reward[i].second<<std::endl;
	    std::cout<<std::endl;*/


	  
	  _reset_prop = gen_reset_prop(reward, _proportions);
	  
	  update_history(reward);
	  
	  auto running = get_still_running(reward);

	  pop.clear();
	  //std::cout<<"init size: "<<pop.size()<<std::endl;
	  _S =  select_emitters(running);

	  _S.insert(_S.end(), running.begin(), running.end());
	  
	  /*std::cout<<"W: ";
	  for(size_t i=0;i<_reward.size();i++)
	    std::cout<< _reward[i] <<" ";
	  std::cout<<std::endl;

	  std::cout<<"P: ";
	  for(size_t i=0;i<_p.size();i++)
	    std::cout<< _p[i] <<" ";
	  std::cout<<std::endl;
	  */
	  _proportions.clear();
	  _proportions.resize(sizeof...(Emitters), 0);
	  
	  //std::cout<<"S: ";
	  for(size_t i=0;i<_S.size();i++){
	    _proportions[(size_t) std::floor(_S[i]/Params::Emitter_pool::nb_emitters)]+=(1.0f/_S.size());
	    //std::cout<< _S[i] <<" ";
	  }
	  //std::cout<<std::endl;

	  /*std::cout<<"proportions: ";
	  for(size_t i=0;i<_proportions.size();i++){
	    std::cout<< _proportions[i] <<" ";
	    }
	  std::cout<<std::endl;
	  */
	  
	  it=0;
	  boost::fusion::for_each(_emitter_pool,UCB_form_pop<Phen, EA>(pop,ea, _S, it));
	  //std::cout<<"final size: "<<pop.size()<<std::endl;
	}

	const std::vector<float>& get_proportions() const{
	  return _proportions;
	}

	const std::vector<float>& get_reset_prop() const{
	  return _reset_prop;
	}

      private:
	boost::fusion::vector< std::vector<std::shared_ptr<Emitters> >... > _emitter_pool;
	std::vector<float> _proportions;
	std::vector<float> _reset_prop;
	boost::circular_buffer<std::vector<std::pair<size_t,float> > > _history;
	std::vector<size_t> _S;
	size_t _nb_emitters;
	float _tau=0.05;//0.0005;

	
	void update_history( std::vector<std::pair<size_t,float>>& reward){
	  std::vector<std::pair<size_t,float> > frame(_nb_emitters, {0,0});
	  for(size_t i=0; i<reward.size(); i++){
	    size_t index = reward[i].first;
	    float val = reward[i].second;
	    frame[index]={1,val};
	  }
	  _history.push_back(frame);
	}

	std::vector<float> gen_reset_prop(const std::vector<std::pair<size_t,float>>& reward, const std::vector<float>& proportions) const{
	  std::vector<float> reset_prop(sizeof...(Emitters), 0);
	  for(size_t i=0;i<reward.size();i++){
	    size_t type = (size_t) std::floor(reward[i].first/Params::Emitter_pool::nb_emitters);
	    reset_prop[ type ] += (1 / (proportions[type] * Params::Emitter_pool::nb_emitters));
	  }
	  for(size_t i=0;i<proportions.size();i++)
	    if(proportions[i]< 1e-4)
	      reset_prop[ i ] = 1;
	  
	  return reset_prop;
	  
	}

	std::vector<size_t> get_still_running(const std::vector<std::pair<size_t,float>>& reward){
	  std::vector<size_t> res=_S;
	  for(auto i:reward){
	    
	    auto it = std::find(res.begin(), res.end(), i.first);
	    if(it!= res.end()){
	      res.erase(it);
	    }
	    else
	      assert(0); // there should be an element here
	  }
	  return res;
	}
	


	std::vector<size_t> select_emitters(std::vector<size_t> running){ // UCB1 ALGORITHM
	  //std::cout<<"start selection"<<std::endl;
	  
	  if(running.size() == Params::Emitter_pool::nb_emitters){
	    std::vector<size_t> res;
	    return res;
	  }
	  
	  if(Params::Emitter_pool::nb_emitters == _nb_emitters )// no selection needed;
	    {
	      std::vector<size_t> res;
	      for(size_t i=0; i<_nb_emitters; i++)
		res.push_back(i);
	      return res;
	    }


	  //std::cout<<"GENERAL CASE"<<std::endl;
	  // general case
	  
	  std::vector<float> temp_reward(_nb_emitters,0.0f);
	  std::vector<float> temp_sel(_nb_emitters,0.0f);
	  for(size_t i=0; i<_history.size(); i++)
	    for(size_t j=0; j<_nb_emitters;j++)
	      {
		temp_sel[j] +=_history[i][j].first;
		temp_reward[j] +=_history[i][j].second;
	      }
	  size_t nb_sel =  std::accumulate(temp_sel.begin(), temp_sel.end(), 0);
	  //std::cout<<nb_sel;
	  for(size_t i=0; i<_nb_emitters; i++)
	    {
	      //if(i%Params::Emitter_pool::nb_emitters ==0)
	      //std::cout<<std::endl;

	      if(temp_sel[i])
		temp_reward[i]= temp_reward[i]/(float)temp_sel[i] + _tau*sqrt(2*log((float)nb_sel)/temp_sel[i]);
	      else
		temp_reward[i]= std::numeric_limits<float>::infinity();
	      //std::cout<<"R"<<i <<":"<<temp_reward[i]<<"-"<< temp_sel[i]<<" ";
	    }
	  //std::cout<<std::endl;
	  //std::cout<<std::endl;
	  
	  for(size_t i: running)
	    temp_reward[i]=0; //deactivate emitters that are currently running
	  auto res = reverse_sort_indexes(temp_reward);
	  res.resize(Params::Emitter_pool::nb_emitters - running.size());
	  return res;
	  
	  
	      
	}

	template <typename T>
	std::vector<size_t> reverse_sort_indexes(const std::vector<T> &v) {
	  
	  // initialize original index locations
	  std::vector<size_t> idx(v.size());
	  std::iota(idx.begin(), idx.end(), 0);
	  
	  // sort indexes based on comparing values in v
	  // using std::stable_sort instead of std::sort
	  // to avoid unnecessary index re-orderings
	  // when v contains elements of equal values 
	  std::partial_sort(idx.begin(), idx.begin() + Params::Emitter_pool::nb_emitters, idx.end(),
		      [&v](size_t i1, size_t i2) {return v[i1] > v[i2];});
	  
	  return idx;
	}
	  
	
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
