#ifndef QD_EMITTER_POOL_HPP
#define QD_EMITTER_POOL_HPP

namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Emitter, typename Phen, typename Params> struct Emitter_pool {
	Emitter_pool(): _emitter_pool_count(Params::Emitter_pool::nb_emitters,0)
	{
	  assert(std::round((float)Params::pop::size/(float)Params::Emitter_pool::nb_emitters)==Params::pop::size/Params::Emitter_pool::nb_emitters);
	  for(size_t i=0; i<Params::Emitter_pool::nb_emitters; i++)
	    _emitter_pool.push_back(std::make_shared<Emitter>());
	}
	
	typedef boost::shared_ptr<Phen> indiv_t;
	
	template <typename EA>
	void operator()(std::vector<indiv_t>& pop, const EA& ea) 
	{
	  assert(pop.size()==0 || pop.size()==ea.added().size());

	  for(size_t i=0; i <pop.size(); i++){
	    //pop[i]->fit().added=ea.added()[i];
	    pop[i]->fit().added=std::get<0>(ea._updates[i]);
	    pop[i]->fit().new_cell=std::get<1>(ea._updates[i]);
	    pop[i]->fit().improvement=std::get<2>(ea._updates[i]);
	  }
	  pop.clear();
	  for (auto& emit : _emitter_pool)
	    {
	      emit->update_batch();
	      emit->gen_batch(ea);
	      pop.insert(pop.end(), emit->get_batch().begin(), emit->get_batch().end());
	    }
	}

      private:
	std::vector< std::shared_ptr<Emitter> > _emitter_pool;
	std::vector< size_t> _emitter_pool_count; // counting the number of element selected from each emitter
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
