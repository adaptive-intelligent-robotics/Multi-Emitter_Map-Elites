#ifndef QD_CMA_EMITTER_BASE_HPP
#define QD_CMA_EMITTER_BASE_HPP


#include <sferes/ea/cmaes_interface.h>
namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Phen, typename Params>
      struct CMA_emitter_base {

	typedef boost::shared_ptr<Phen> indiv_t;

	CMA_emitter_base():_need_reset(true), _nb_evals(0), _nb_addeds(0), _nb_episodes(0){
	  _cmaes.version = NULL;
	  _pop.clear();
	}
	virtual ~CMA_emitter_base() {
	  if(_cmaes.version)
	    {
	    cmaes_exit(&_cmaes);
	    _cmaes.version = NULL;
	    }
	}
	
	std::vector<indiv_t>& get_batch(){
	  return this->_pop;
	}
	
	void update_batch()
	{

	  //check wether it is the first round or not
	  if(_pop.size())
	    {
	      this->assign_value();
	      cmaes_UpdateDistribution(&_cmaes, _ar_funvals);
	      // update values for score
	      for(auto it = this->get_batch().begin(); it!= this->get_batch().end(); it++)
		  if( (*it)->fit().added)
		    _nb_addeds++;
	      _nb_evals+=this->get_batch().size();
	      _nb_episodes++;
	      
	      if(this->check_termination())
		_need_reset=true;
	    }
	}
	
	template <typename EA>
	void gen_batch( const EA& ea)
	{

	  // reset if needed
	  if(_need_reset)
	    reset(ea);

	  _cmaes_pop = cmaes_SamplePopulation(&_cmaes);

	  this->_pop.clear();

	  for (size_t i = 0; i <  _batch_size; ++i){
	    this->_pop.push_back(boost::shared_ptr<Phen>(new Phen()));
	    for (size_t j = 0; j < this->_pop[i]->gen().size(); ++j)
	      {
		this->_pop[i]->gen().data(j, std::min(1.0f, std::max(0.0f,(float) _cmaes_pop[i][j])));
		_cmaes_pop[i][j]=std::min(1.0f, std::max(0.0f,(float) _cmaes_pop[i][j]));
	      }
	    this->_pop[i]->develop();
          }

	}


	
	template <typename EA>
	void reset(const EA& ea)
	{

	  _need_reset = false;
	  _nb_evals = 0;
	  _nb_addeds = 0;
	  _nb_episodes = 0;


	  int x1 = misc::rand<int>(0, ea.pop().size());
	  auto indiv =  ea.pop()[x1];
	  dim = indiv->gen().size();

	  auto xinit = cmaes_NewDouble(dim);
	  auto siginit = cmaes_NewDouble(dim);

	  double sig=Params::Emitter_pool::sig_init; // was 0.1 in previous implementation
	  for (size_t i=0;i<dim;i++){
	    if(ea.gen()<=1)
	      xinit[i]=0; // this is how the original CMA-ME initialises its emitters... this is an ugly cheat, but without this (i.e. with uniform init) it does not work.... 
	    else
	      xinit[i]=indiv->gen().data(i);

	    siginit[i]=sig;
	  }

	  if (_cmaes.version){

	    cmaes_exit(&_cmaes);
	    _cmaes.version = NULL;

	  }

	  _ar_funvals = cmaes_init(&_cmaes, dim, xinit, siginit, 0,  _batch_size, NULL);
	  this->specific_reset();

	}

	//float get_score(){return (float)_nb_episodes * (float)_nb_addeds/(float)_nb_evals;}
	float get_score(){return (float)_nb_addeds/(float)_nb_evals;}

	virtual bool check_termination()=0;
	
      protected:
	virtual void assign_value()=0;

	virtual void specific_reset()=0;
	
	size_t _batch_size = Params::pop::size/Params::Emitter_pool::nb_emitters;
	std::vector<indiv_t> _pop;
	bool _need_reset;
	size_t dim;
	double *_ar_funvals;
	double * const * _cmaes_pop;
	int _lambda;
	cmaes_t _cmaes;

	size_t _nb_evals;
	size_t _nb_addeds;
	size_t _nb_episodes;
	
      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
