#ifndef QD_IMPROVEMENT_EMITTER_HPP
#define QD_IMPROVEMENT_EMITTER_HPP

#include <limits>
#include <sferes/ea/cmaes_interface.h>
namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Phen, typename Params>
      struct Improvement_emitter: public CMA_emitter_base<Phen, Params> {

	Improvement_emitter():CMA_emitter_base<Phen,Params>(){}

	bool check_termination() override {
	  return (cmaes_TestForTermination(&(this->_cmaes)) || count_added()==0);
	}

	
      private:
	void assign_value() override {
	  //std::cout<<"Value: IMP"<<std::endl;
	  size_t new_cell=0;
	  size_t improvement=0;
	  double inf = std::numeric_limits<double>::infinity();
	  double largest_improvment_existing_cell=-inf;
	  double smallest_improvment_new_cell=inf;
	  bool has_new_cell = false;
	  for (size_t i = 0; i < this->_pop.size(); i++) 
	    if(this->_pop[i]->fit().added && !this->_pop[i]->fit().new_cell &&
	       largest_improvment_existing_cell < this->_pop[i]->fit().improvement)
	      largest_improvment_existing_cell=this->_pop[i]->fit().improvement;
	    else if(this->_pop[i]->fit().added && this->_pop[i]->fit().new_cell &&
		     smallest_improvment_new_cell > this->_pop[i]->fit().improvement)
	      {
		has_new_cell = true;
		smallest_improvment_new_cell=this->_pop[i]->fit().improvement;
	      }

	  if(! has_new_cell)
	    smallest_improvment_new_cell = 0;
	  
	  for (size_t i = 0; i < this->_pop.size(); i++) {
	    //warning: CMAES minimizes the fitness...
	    if(!this->_pop[i]->fit().added)
	      this->_ar_funvals[i] = inf; //to be ranked last (carefule CMAES minimizes
	    else if(!this->_pop[i]->fit().new_cell){
	      this->_ar_funvals[i] = largest_improvment_existing_cell - this->_pop[i]->fit().improvement - smallest_improvment_new_cell + 1;
	      improvement++;
	    }
	    else {
	      this->_ar_funvals[i] = -this->_pop[i]->fit().improvement; // to be ranked above the existing cells
	      new_cell++;
	    }
	    //std::cout<<this->_pop[i]->fit().added<<" "<<this->_pop[i]->fit().new_cell<<" "<<this->_pop[i]->fit().improvement<<" "<<this->_ar_funvals[i]<<std::endl;

	  }

	  //std::cout<<"new_cell: "<<new_cell<<" improvement: "<<improvement<<std::endl;
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
