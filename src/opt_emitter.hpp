#ifndef QD_OPT_EMITTER_HPP
#define QD_OPT_EMITTER_HPP


#include <sferes/ea/cmaes_interface.h>
#include "cma_emitter_base.hpp"

namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Phen, typename Params>
      struct Opt_emitter: public CMA_emitter_base<Phen, Params> {

	Opt_emitter():CMA_emitter_base<Phen,Params>(){}

	bool check_termination() override {
	  return (cmaes_TestForTermination(&(this->_cmaes)) || count_added()==0);
		  
	  //return cmaes_TestForTermination(&(this->_cmaes)); //returns null if not finished
	}

	
      private:
	void assign_value() override {
	  //std::cout<<"Value: OPT"<<std::endl;
	  for (size_t i = 0; i < this->_pop.size(); ++i)
	    //warning: CMAES minimizes the fitness...
	    this->_ar_funvals[i] = - this->_pop[i]->fit().value();
	}

	size_t count_added(){
	  size_t count =0;
	  for (size_t i = 0; i < this->_pop.size(); i++)
	    if(this->_pop[i]->fit().added)
	      count++;
	  return count;
	}
	
	
	void specific_reset() override {}

	
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
