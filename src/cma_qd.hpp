//| This file is a part of the sferes2 framework.
//| Copyright 2009, ISIR / Universite Pierre et Marie Curie (UPMC)
//| Main contributor(s): Jean-Baptiste Mouret, mouret@isir.fr
//|
//| This software is a computer program whose purpose is to facilitate
//| experiments in evolutionary computation and evolutionary robotics.
//|
//| This software is governed by the CeCILL license under French law
//| and abiding by the rules of distribution of free software.  You
//| can use, modify and/ or redistribute the software under the terms
//| of the CeCILL license as circulated by CEA, CNRS and INRIA at the
//| following URL "http://www.cecill.info".
//|
//| As a counterpart to the access to the source code and rights to
//| copy, modify and redistribute granted by the license, users are
//| provided only with a limited warranty and the software's author,
//| the holder of the economic rights, and the successive licensors
//| have only limited liability.
//|
//| In this respect, the user's attention is drawn to the risks
//| associated with loading, using, modifying and/or developing or
//| reproducing the software by the user in light of its specific
//| status of free software, that may mean that it is complicated to
//| manipulate, and that also therefore means that it is reserved for
//| developers and experienced professionals having in-depth computer
//| knowledge. Users are therefore encouraged to load and test the
//| software's suitability as regards their requirements in conditions
//| enabling the security of their systems and/or data to be ensured
//| and, more generally, to use and operate it in the same conditions
//| as regards security.
//|
//| The fact that you are presently reading this means that you have
//| had knowledge of the CeCILL license and that you accept its terms.

#ifndef CMA_QD_HPP_
#define CMA_QD_HPP_

#include <algorithm>
#include <limits>

#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/multi_array.hpp>
#include <boost/timer/timer.hpp>

#include <sferes/ea/ea.hpp>
#include <sferes/fit/fitness.hpp>
#include <sferes/stc.hpp>

#include <sferes/qd/container/cvt.hpp>
#include <sferes/qd/container/grid.hpp>
#include <sferes/qd/container/sort_based_storage.hpp>
#include <sferes/qd/selector/uniform.hpp>

namespace sferes {
  namespace qd {
    
    // Main class
    template <typename Phen, typename Eval, typename Stat, typename FitModifier,
	      typename Selector, typename Container, typename Params, typename Exact = stc::Itself>
    class CMA_QD
      : public qd::QualityDiversity<Phen, Eval, Stat, FitModifier, Selector, Container, Params,
				    typename stc::FindExact<CMA_QD<Phen, Eval, Stat, FitModifier, Selector,
                                              Container, Params, Exact>,
							    Exact>::ret> {
    public:
      typedef Phen phen_t;
      typedef boost::shared_ptr<Phen> indiv_t;
      typedef typename std::vector<indiv_t> pop_t;
      typedef typename pop_t::iterator it_t;
      
      CMA_QD() {}
      
      // Random initialization of _parents and _offspring
      void random_pop()
      {
	parallel::init();
	
	this->_pop.clear();
	
	this->_offspring.resize(Params::pop::size);
	BOOST_FOREACH (indiv_t& indiv, this->_offspring) {
	  indiv = indiv_t(new Phen());
	  indiv->random();
	}
	this->_eval_pop(this->_offspring, 0, this->_offspring.size());
	this->apply_modifier();
	
	this->add_v2(this->_offspring, this->_added, this->_updates);
	
	this->_parents = this->_offspring;
	this->_offspring.resize(Params::pop::size);
	
	BOOST_FOREACH (indiv_t& indiv, this->_offspring) {
	  indiv = indiv_t(new Phen());
	  indiv->random();
	}
	
	this->_eval_pop(this->_offspring, 0, this->_offspring.size());
	this->apply_modifier();
	this->add_v2(this->_offspring, this->_added, this->_updates);

	this->_container.get_full_content(this->_pop);
      }
    
      // Main Iteration of the QD algorithm
      void epoch()
      {

	// parents are not used in CMA_QD
	this->_parents.clear();

	// Selection of the parents (will fill the _parents vector)
	this->_offspring.resize(Params::pop::size);
	//std::cout<<"Sel"<<std::endl;
	this->_selector(this->_offspring, *this); // not a nice API
	
	//std::cout<<"eval"<<std::endl;
	// Evaluation of the offspring
	this->_eval_pop(this->_offspring, 0, this->_offspring.size());
	//std::cout<<"modif"<<std::endl;
	this->apply_modifier();
	
	//std::cout<<"add"<<std::endl;
	// Addition of the offspring to the container
	this->add_v2(this->_offspring, this->_added, this->_updates);
	
	//std::cout<<"clear"<<std::endl;
	this->_pop.clear();
	//std::cout<<"container"<<std::endl;
	// Copy of the containt of the container into the _pop object.
	this->_container.get_full_content(this->_pop);
	//std::cout<<"end"<<std::endl;
      }
      
      // Same function, but without the need of parent.
      void add_v2(pop_t& pop_off,std::vector<bool>& added, std::vector<std::tuple<bool, bool, float>>& updates)
      {
	updates.resize(pop_off.size());
	added.resize(pop_off.size());
	for (size_t i = 0; i < pop_off.size(); ++i){
	  updates[i] = this->_container.add_v2(pop_off[i]);
	  added[i]=std::get<0>(updates[i]);
	}
	pop_t empty;
	this->_container.update(pop_off, empty);
      }


      const Selector& selector() const { return this->_selector; }
      std::vector<std::tuple<bool, bool, float>> _updates;
    };
    
  } // namespace qd
} // namespace sferes
#endif
