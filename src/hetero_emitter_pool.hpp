#ifndef QD_HETERO_EMITTER_POOL_HPP
#define QD_HETERO_EMITTER_POOL_HPP

namespace sferes {
  namespace qd {
    namespace selector {

      struct init_pool
      {
	init_pool(size_t size):_size(size){}
	
	template <typename T>
	void operator()(T &subpool) const
	{
	  for(size_t i=0; i<_size; i++)
	    subpool.push_back(std::make_shared<typename T::value_type::element_type>());
	}

	size_t _size;
      };

      template<typename Phen, typename EA>
      struct form_pop
      {
	typedef boost::shared_ptr<Phen> indiv_t;
	
	form_pop(std::vector<indiv_t>& pop, const EA& ea):_pop(pop), _ea(ea){}
	

	template <typename T>
	void operator()(T &subpool) const
	{
	  for (auto& emit : subpool)
	    {
	      

	      emit->update_batch();

	      emit->gen_batch(_ea);

	      _pop.insert(_pop.end(), emit->get_batch().begin(), emit->get_batch().end());

	    }
	}
	std::vector<indiv_t>& _pop;
	const EA& _ea;
      };
      
      template <typename Phen, typename Params, typename ... Emitters >
      struct Hetero_Emitter_pool {
	Hetero_Emitter_pool()//: _emitter_pool_count(Params::Emitter_pool::nb_emitters,0)
	{
	  assert(std::round((float)Params::pop::size/(float)Params::Emitter_pool::nb_emitters)==Params::pop::size/Params::Emitter_pool::nb_emitters);
	  assert(std::round((float)Params::Emitter_pool::nb_emitters / sizeof...(Emitters))==Params::Emitter_pool::nb_emitters / sizeof...(Emitters));
	  std::cout<<"Number of emitter types: "<<sizeof...(Emitters)<<std::endl;
	  boost::fusion::for_each(_emitter_pool,init_pool(Params::Emitter_pool::nb_emitters / sizeof...(Emitters) ));
	}
	
	typedef boost::shared_ptr<Phen> indiv_t;
	
	template <typename EA>
	void operator()(std::vector<indiv_t>& pop, const EA& ea) 
	{

		  
	  assert(pop.size()==0 || pop.size()==ea.added().size());

	  for(size_t i=0; i <pop.size(); i++){
	    pop[i]->fit().added=std::get<0>(ea._updates[i]);
	    pop[i]->fit().new_cell=std::get<1>(ea._updates[i]);
	    pop[i]->fit().improvement=std::get<2>(ea._updates[i]);
	  }
	  pop.clear();

	  boost::fusion::for_each(_emitter_pool,form_pop<Phen, EA>(pop,ea));
	  //std::cout<<"final pop size: "<<pop.size()<<std::endl;
	}

      private:
	boost::fusion::vector< std::vector<std::shared_ptr<Emitters> >... > _emitter_pool;
	//std::vector< size_t> _emitter_pool_count; // counting the number of element selected from each emitter. NO USED??!!
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
