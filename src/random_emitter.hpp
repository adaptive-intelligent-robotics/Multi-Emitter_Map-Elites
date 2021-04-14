#ifndef QD_RANDOM_EMITTER_HPP
#define QD_RANDOM_EMITTER_HPP


#include <sferes/ea/cmaes_interface.h>
namespace sferes {
  namespace qd {
    namespace selector {
      // MAP-Elites style: select size(pop) elites
      template <typename Phen, typename Params>
      struct Random_emitter{

	typedef boost::shared_ptr<Phen> indiv_t;

	Random_emitter(): _nb_evals(0), _nb_addeds(0) {
	  _pop.clear();
	}
	
	std::vector<indiv_t>& get_batch(){
	  return this->_pop;
	}

	void update_batch(){
	  _nb_addeds=0;
	  _nb_evals=0;
	  for(auto it = this->get_batch().begin(); it!= this->get_batch().end(); it++)
	    if( (*it)->fit().added)
	      _nb_addeds++;
	  _nb_evals+=this->get_batch().size();
	}
	
	template <typename EA>
	void gen_batch( const EA& ea)
	{
	  _parents.resize(_batch_size);
	  _pop.resize(_batch_size);
	  for (auto& indiv : _parents) {
	    int x1 = misc::rand<int>(0, ea.pop().size());
	    indiv = ea.pop()[x1];
	  }

	  
	  // Generation of the offspring
	  std::vector<size_t> a;
	  misc::rand_ind(a, _parents.size());
	  assert(_parents.size() == _batch_size);
	  assert(_batch_size%2==0);
	  for (size_t i = 0; i < _batch_size; i += 2) {
	    boost::shared_ptr<Phen> i1, i2;
	    _parents[a[i]]->cross(_parents[a[i + 1]], i1, i2);
	    i1->mutate();
	    i2->mutate();
	    i1->develop();
	    i2->develop();
	    _pop[a[i]] = i1;
	    _pop[a[i + 1]] = i2;

	    /*float d11=0;
	    float d12=0;
	    float d21=0;
	    float d22=0;
	    
	    for( size_t j=0; j<i1->gen().size(); j++)
	      {
		d11 += (_parents[a[i]]->gen().data(j) - i1->gen().data(j))*(_parents[a[i]]->gen().data(j) - i1->gen().data(j));
		d12 += (_parents[a[i]]->gen().data(j) - i2->gen().data(j))*(_parents[a[i]]->gen().data(j) - i2->gen().data(j));
		d21 += (_parents[a[i+1]]->gen().data(j) - i1->gen().data(j))*(_parents[a[i+1]]->gen().data(j) - i1->gen().data(j));
		d22 += (_parents[a[i+1]]->gen().data(j) - i2->gen().data(j))*(_parents[a[i+1]]->gen().data(j) - i2->gen().data(j));
	      }
	    std::cout<<"d11:"<<d11<<" d12:"<<d12<<" d21:"<<d21<<" d22:"<<d22<<std::endl;
	    */
	  }

	  
	}
	bool check_termination(){return true;}
	float get_score(){return (float)_nb_addeds/(float)_nb_evals;}

	
	
      protected:
	size_t _batch_size = Params::pop::size/Params::Emitter_pool::nb_emitters;
	std::vector<indiv_t> _pop;
	std::vector<indiv_t> _parents;

	size_t _nb_evals;
	size_t _nb_addeds;


      };
    } // namespace selector
  } // namespace qd
} // namespace sferes
#endif
