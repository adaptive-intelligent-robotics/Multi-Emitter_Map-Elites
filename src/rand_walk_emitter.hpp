#ifndef QD_RD_WALK_EMITTER_HPP
#define QD_RD_WALK_EMITTER_HPP


#include <sferes/ea/cmaes_interface.h>
namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Phen, typename Params>
      struct Rand_walk_emitter: public CMA_emitter_base<Phen, Params> {

	Rand_walk_emitter():CMA_emitter_base<Phen,Params>(){}

	bool check_termination() override {
	  return ( cmaes_TestForTermination(&(this->_cmaes)) || count_added()==0 );
	}

	
      private:
	void assign_value() override {
	  //std::cout<<"Value: RDW"<<std::endl;
	  _mean=Eigen::VectorXd::Zero(_v.size());
	  for (size_t i = 0; i < this->_pop.size(); i++) 
	    for (size_t j = 0; j < _v.size(); ++j) 
	      _mean[j]+=this->_pop[i]->fit().desc()[j];
	  _mean/=this->_pop.size();
	  
	  for (size_t i = 0; i < this->_pop.size(); i++) {
	    Eigen::VectorXd v(_v.size());
	    for (size_t j = 0; j < _v.size(); ++j) 
	      v[j]=this->_pop[i]->fit().desc()[j] - _mean[j];

	    //warning: CMAES minimizes the fitness...
	    this->_ar_funvals[i] = - _v.dot(v);
	  }

	}

	

	size_t count_added(){
	  size_t count =0;
	  for (size_t i = 0; i < this->_pop.size(); i++)
	    if(this->_pop[i]->fit().added)
	      count++;
	  return count;
	}
	

	void specific_reset() override
	{
	  _v=Eigen::VectorXd::Random(Params::qd::behav_dim);
	  _v/=_v.norm();
	}

	Eigen::VectorXd _v;
	Eigen::VectorXd _mean;
	
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
