#ifndef QD_MAB_EMITTER_POOL_HPP
#define QD_MAB_EMITTER_POOL_HPP

namespace sferes {
  namespace qd {
    namespace selector {

      struct MAB_init_pool
      {
	MAB_init_pool(size_t size):_size(size){}
	
	template <typename T>
	void operator()(T &subpool) const
	{
	  std::cout<<"here1"<<std::endl;
	  for(size_t i=0; i<_size; i++)
	    subpool.push_back(std::make_shared<typename T::value_type::element_type>());
	}

	size_t _size;
      };

      template<typename Phen, typename EA>
      struct MAB_form_pop
      {
	typedef boost::shared_ptr<Phen> indiv_t;
	
	MAB_form_pop(std::vector<indiv_t>& pop, const EA& ea, const std::vector<size_t>& S, size_t& it):_pop(pop), _ea(ea),_it(it), _S(S){}
	

	template <typename T>
	void operator()(T &subpool) const
	{
	  for (auto& emit : subpool)
	    {
	      if(std::find(_S.begin(), _S.end(), _it) != _S.end()){
		emit->update_batch();
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


      struct MAB_collect_reward
      {
	MAB_collect_reward(std::vector<std::pair<size_t,float>>& reward, const std::vector<size_t>& S, size_t& it):_reward(reward), _it(it), _S(S){
	}
	

	template <typename T>
	void operator()(T &subpool) const
	{
	  for (auto& emit : subpool)
	    {
	      if(std::find(_S.begin(), _S.end(), _it) != _S.end()){
		float val = 0;
		for(auto it = emit->get_batch().begin(); it!= emit->get_batch().end(); it++)
		  if( (*it)->fit().added)
		    val ++;
		val /=(float) emit->get_batch().size();
		_reward.push_back({_it, val});
	      }
	      _it++;
	    }

	}
	std::vector<std::pair<size_t, float>>& _reward;
	size_t& _it;
	const std::vector<size_t>& _S;
      };

      
      

      
      template <typename Phen, typename Params, typename ... Emitters >
      struct MAB_Emitter_pool {
	MAB_Emitter_pool():_w( Params::Emitter_pool::nb_emitters * sizeof...(Emitters), 1.0), _alpha(-1)
	{
	  std::cout<<"Number of emitter types: "<<sizeof...(Emitters)<<std::endl;
	  std::cout<<"Number of emitters: "<<_w.size()<<std::endl;
	  
	  boost::fusion::for_each(_emitter_pool,MAB_init_pool(Params::Emitter_pool::nb_emitters));
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
	  boost::fusion::for_each(_emitter_pool,MAB_collect_reward(reward,_S,it));
	  /*for(size_t i=0;i<reward.size();i++)
	    std::cout<< reward[i].first <<" "<<reward[i].second<<std::endl;
	    std::cout<<std::endl;*/

	  update_weights(reward);
	  
	  
	  pop.clear();
	  //std::cout<<"init size: "<<pop.size()<<std::endl;
	  _S =  select_emitters();

	  /*std::cout<<"W: ";
	  for(size_t i=0;i<_w.size();i++)
	    std::cout<< _w[i] <<" ";
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
	    }*/
	  //std::cout<<std::endl;

	  
	  it=0;
	  boost::fusion::for_each(_emitter_pool,MAB_form_pop<Phen, EA>(pop,ea, _S, it));
	  //std::cout<<"final size: "<<pop.size()<<std::endl;
	}

	const std::vector<float>& get_proportions() const{
	  return _proportions;
	}
	
      private:
	boost::fusion::vector< std::vector<std::shared_ptr<Emitters> >... > _emitter_pool;
	std::vector<float> _proportions;
	std::vector<float> _w;
	std::vector<float> _p;
	std::vector<size_t> _S;
	float _alpha;
	float _gamma = 0.15;
	float lambda = 0.0002;
	
	void update_weights( std::vector<std::pair<size_t,float>>& reward){
	  float sum  = std::accumulate(_w.begin(), _w.end(), 0.0f);
	  float constant = lambda * exp(1) * sum /(float)_w.size();
	  for(size_t i=0; i<reward.size(); i++){
	    size_t index = reward[i].first;
	    float val = reward[i].second;	    
	    if(_alpha < 0 || _w[index] != _alpha) {
	      //std::cout<<index<<" : "<<val<<std::endl;
	      //_w[index]=_w[index]*exp((float)Params::Emitter_pool::nb_emitters * _gamma * val/_p[index] / (float)_w.size());
	      _w[index]=_w[index]*exp((float)Params::Emitter_pool::nb_emitters * _gamma * val/_p[index] / (float)_w.size());
	    }
	  }
	  sum  = std::accumulate(_w.begin(), _w.end(), 0.0f);
	  for(auto& w:_w)
	    w=(w+constant) /(sum + constant*(float)_w.size());

	  //std::cout<<"print W "<<std::accumulate(_w.begin(), _w.end(), 0.0f)<<std::endl;
	}
	

	std::vector<size_t> select_emitters(){ // Exp3.M algorithms
	  if(Params::Emitter_pool::nb_emitters == _w.size() )// no selection needed;
	    {
	      std::vector<size_t> res;
	      for(size_t i=0; i<_w.size(); i++)
		res.push_back(i);
	      return res;
	    }

	  //std::cout<<"start selection"<<std::endl;
	  auto temp_w = _w;
	  if( *std::max_element(_w.begin(), _w.end()) >= (1.0/(float)Params::Emitter_pool::nb_emitters - _gamma/(float)_w.size())/(1.0-_gamma) *  std::accumulate(_w.begin(), _w.end(), 0.0f))  {
	    //std::cout<<"Threshold required"<<std::endl;
	    
	    _alpha = new_alpha(temp_w);
	    
	    temp_w = _w;
	    for(float& wi:temp_w){
	      if(wi>=_alpha)
		wi=_alpha;
	    }
	  }
	  else {
	    _alpha=-1;
	    //std::cout<<"Threshold NOT required"<<std::endl;
	  }
	  //std::cout<<"compute proba"<<std::endl;

	  _p.clear();
	  _p.resize(_w.size(),0);
	  float sum =  std::accumulate(temp_w.begin(), temp_w.end(), 0.0f);
	  /*std::cout<<_w.size()<<": ";
	  for(size_t i=0; i<_w.size(); i++)
	    {
	      std::cout<<_w[i]<<" ";
	    }
	  std::cout<<std::endl;
	  */
	  //std::cout<<_p.size()<<": ";
	  for(size_t i=0; i<_w.size(); i++)
	    {
	      _p[i]= Params::Emitter_pool::nb_emitters * ( (1-_gamma)*temp_w[i]/sum  + _gamma/(float)_w.size() );
	      //  std::cout<<_p[i]<<" ";
	    }
	  //std::cout<<std::endl;

	  return dep_round( Params::Emitter_pool::nb_emitters,  _p);
	  
	      
	}
	
	float new_alpha(std::vector<float> temp_w)const{
	  std::sort(temp_w.begin(), temp_w.end()); //sort in ascending order
	  float B =  (1.0/(float)Params::Emitter_pool::nb_emitters - _gamma/(float)_w.size())/(1.0-_gamma);
	  for(int i =temp_w.size()-1; i>=1;i--){
	    //std::cout<<"i: "<<i<<std::endl;
	    
	    auto vi = temp_w.begin();
	    std::advance(vi, i);
	    float A = std::accumulate(temp_w.begin(), vi, 0.0f);
	    float alpha = B*A/(float)(1-B*(temp_w.size()-i));

	    vi = temp_w.begin();
	    std::advance(vi, i-1);
	    //std::cout<<alpha<<" : "<<*vi<<" nalpha +A "<<A+alpha*(temp_w.size()-i)<<std::endl;
	    if(alpha > *vi)
	      {
		    return alpha;
	      }
	  }
	  
	  std::cout<<"alpha not found"<<std::endl;
	  assert(0);
	  // should not reach this
	  return -2;
	}
	  
	/*	bool check_alpha(float alpha)const {
	  //std::cout<<"check alpha"<<std::endl; 
	  float sum = 0;
	  for(float wi:_w){
	    if(wi>=alpha)
	      sum+=alpha;
	    else
	      sum+=wi;
	  }
	  std::cout<<"done alpha"<<std::endl;
	  std::cout<<"sum "<<sum<<std::endl; 
	  std::cout<<alpha/sum<<" vs "<<  (1.0/(float)Params::Emitter_pool::nb_emitters - _gamma/(float)_w.size())/(1.0-_gamma)<<std::endl;
	  return false;
	    
	  }*/
	
	std::vector<size_t> dep_round(size_t k, std::vector<float> p) const
	{
	  //std::cout<<"dep_round"<<std::endl;

	  std::vector<size_t> running_index = not_0_1(p);
	  while(running_index.size() !=0)
	    {
	      
	      /*std::cout << running_index.size() << ": ";
	      for(size_t ii=0; ii<p.size(); ii++)
		std::cout<<p[ii]<<" ";
		std::cout<<std::endl;*/

	      if(running_index.size()==1){
		//float accu  = std::accumulate(p.begin(), p.end(), 0.0f);
		//std::cout<<running_index[0]<<" accu "<<accu<<std::endl;
		/*if( accu <= (float)Params::Emitter_pool::nb_emitters)
		  p[running_index[0]]=1;
		else
		p[running_index[0]]=0;*/
		p[running_index[0]]=	std::round(p[running_index[0]]);
		break;
	      }
	      
	      size_t i = running_index[misc::rand<int>(0, running_index.size())];
	      size_t j=i;
	      while(j==i)
		j= running_index[misc::rand<int>(0, running_index.size())];

	      float alpha = std::min(1.0f - p[i],        p[j]);
	      float beta = std::min(        p[i], 1.0f - p[j]);
	      float coin = misc::rand<float>(0, 1);
	      if( coin < beta/(alpha+beta)){
		p[i]=p[i]+alpha;
		p[j]=p[j]-alpha;
	      }
	      else{
		p[i]=p[i]-beta;
		p[j]=p[j]+beta;
	      }
	      running_index = not_0_1(p);
	    }
	  
	  std::vector<size_t> res;
	  for(size_t i=0;i<p.size();i++)
	    if(!(p[i]<1))
	      {
	      res.push_back(i);
	      //std::cout<<i<<" ";
	      }
	  //std::cout<<std::endl;
	  assert(res.size()==k);
	  return res;
	}
	
	std::vector<size_t> not_0_1(const std::vector<float>& p) const
	{
	  std::vector<size_t> res;
	  for(size_t i=0;i<p.size();i++)
	    if(p[i]>0 && p[i]<1)
	      res.push_back(i);
	  return res;
	}
	
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
