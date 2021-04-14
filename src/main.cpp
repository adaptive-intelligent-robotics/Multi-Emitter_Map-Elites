//| This file is a part of the sferes2 framework.
//| Copyright 2016, ISIR / Universite Pierre et Marie Curie (UPMC)
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

#include <iostream>
#include <Eigen/Core>

#include <sferes/eval/parallel.hpp>
#include <sferes/modif/dummy.hpp>
#include <sferes/run.hpp>
#include <sferes/stat/best_fit.hpp>
#include <sferes/stat/qd_container.hpp>
#include <sferes/stat/qd_selection.hpp>
//#include <sferes/stat/qd_progress.hpp>
#include "cma_progress.hpp"


#include "grid_v2.hpp"
#include <sferes/qd/container/grid.hpp>
#include <sferes/qd/quality_diversity.hpp>
#include <sferes/qd/selector/uniform.hpp>

#include "line_variation.hpp"
#include "stat_qdprintmap.hpp"
#include "stat_mab.hpp"
#include "hetero_emitter_pool.hpp"
#include "mab_emitter_pool.hpp"
#include "fpl_emitter_pool.hpp"
#include "ucb_emitter_pool.hpp"

#include "opt_emitter.hpp"
#include "improvement_emitter.hpp"
#include "rand_walk_emitter.hpp"
#include "random_emitter.hpp"
#include "cma_qd.hpp"

#include "stat_delete_gen_files.hpp"

#ifdef HEXA_UNI
#include "hexapod_unidirectional/hexapod_unidirectional.hpp"
using namespace hexapod_unidirectional;

#elif HEXA_OMNI
#include "hexapod_omnidirectional/hexapod_omnidirectional.hpp"
using namespace hexapod_omnidirectional;

#elif RARM
#include "redundant_arm/redundant_arm.hpp"
using namespace redundant_arm;

#else // RASTRIGIN, RASTRIGIN_MULTI, SPHERE
#include "standard_functions/standard_functions.hpp"
using namespace standard_functions;

#endif






using namespace sferes::gen::evo_float;

struct Params {
  struct Emitter_pool{
    SFERES_CONST size_t nb_emitters = 12;

#if defined(RASTRIGIN) || defined(SPHERE)  
    SFERES_CONST float sig_init = 0.005; // as search space is 100^D, equivalent to sig = 0.5 in phenotype space
#elif RASTRIGIN_MULTI
    SFERES_CONST float sig_init = 0.05; // as search space is 10^D, equivalent to sig = 0.5 in phenotype space
#else // RARM and Hexapods
    SFERES_CONST float sig_init = 0.25; // as search space is 1^D, phenotype and geneotype space are the same.
#endif
    
  };
  
  // TODO: move to a qd::
  struct pop {
    // number of initial random points
    SFERES_CONST size_t init_size = 100;
    // size of a batch
    SFERES_CONST size_t size = 600;
    
    SFERES_CONST size_t nb_gen = 20001;

    SFERES_CONST size_t dump_period = 500;
  };
  
  // variation operator for the random emitter
  struct evo_float: public Params_default::evo_float{
    SFERES_CONST size_t cross_over_type = sferes::gen::evo_float::line; // line variation operator from Vassiliades et al. 2018.
    SFERES_CONST float cross_rate = 1.0f; // the line operator only apply cross-over, which also adds noise on the parameter
    SFERES_CONST float sigma_line = 0.1f; // line variation operator from Vassiliades et al. 2018.
    SFERES_CONST float sigma_iso  = 0.01f; // line variation operator from Vassiliades et al. 2018.
    SFERES_CONST float mutation_rate = 0.0f; // no mutation used
    SFERES_CONST mutation_t mutation_type = polynomial; //not used.
  };
  
  
  /*struct evo_float: public Params_default::evo_float{
    SFERES_CONST float cross_rate = 0.00f;
    //SFERES_CONST float mutation_rate = 1.0f; // no mutation used
    };*/
  
  
  struct nov: public Params_default::nov{};
  struct qd: public Params_default::qd{
    #if defined(RASTRIGIN) || defined(SPHERE)  
    SFERES_ARRAY(size_t, grid_shape, 100, 100);
    #endif
  };

#ifdef RARM
  struct task: Params_default::task{
    SFERES_CONST size_t gen_dim = 100;
  };
#endif


struct parameters: public Params_default::parameters{
#if defined(RASTRIGIN) || defined(SPHERE)  
  SFERES_CONST float min = -51.2;
  SFERES_CONST float max =  51.2;
#endif
};
  
  
#if defined(RASTRIGIN) || defined(RASTRIGIN_MULTI) || defined(SPHERE)  
  struct task: Params_default::task{
    #ifdef RASTRIGIN
    SFERES_CONST Function function = rastrigin;
    #elif RASTRIGIN_MULTI
    SFERES_CONST Function function = rastrigin_multi;
    #else // SPHERE
    SFERES_CONST Function function = sphere;
    #endif
    
    SFERES_CONST size_t gen_dim = 100;
  };

  
#endif
  
};


// Creation of a generic Fitness class to add and init members used by some emitters
class Fitness: public fit_t<Params>
{
  
 public :
  
  Fitness(){
    this->added=false;
    this->new_cell=false;
    this->improvement=0.0;
    
  }
  
  bool added;
  bool new_cell;
  double improvement;
};



int main(int argc, char **argv) 
{
  tbb::task_scheduler_init init(32);
  using namespace sferes;
  
#ifdef HEXA_OMNI
  load_and_init_robot("exp/multi_emitter_map_elites/src/hexapod_omnidirectional/");
#elif HEXA_UNI
  load_and_init_robot("exp/multi_emitter_map_elites/src/hexapod_unidirectional/");
#endif

  typedef  sferes::phen::Parameters<gen_t<Params>, Fitness, Params> phen_t;

    
#ifdef CMAME
  std::cout<<"CMA-ME"<<std::endl;
    
#ifdef OPT
    typedef qd::selector::Opt_emitter<phen_t, Params> emitter_t;
#elif IMP
    typedef qd::selector::Improvement_emitter<phen_t, Params> emitter_t;
#elif  RDW
    typedef qd::selector::Rand_walk_emitter<phen_t, Params> emitter_t;
#elif  RAND
    typedef qd::selector::Random_emitter<phen_t, Params> emitter_t;
#endif
    //typedef qd::selector::Hetero_Emitter_pool< phen_t, Params, emitter_t> select_t;
    typedef qd::selector::Hetero_Emitter_pool< phen_t, Params, emitter_t> select_t;
    typedef qd::container::Grid_V2<phen_t, Params> container_t;
    
#elif HETERO
    std::cout<<"HETERO"<<std::endl;
    typedef qd::selector::Opt_emitter<phen_t, Params> emitter1_t;
    typedef qd::selector::Improvement_emitter<phen_t, Params> emitter2_t;
    typedef qd::selector::Rand_walk_emitter<phen_t, Params> emitter3_t;
    typedef qd::selector::Random_emitter<phen_t, Params> emitter4_t;

#ifdef UCB
    typedef qd::selector::UCB_Emitter_pool< phen_t, Params, emitter1_t, emitter2_t, emitter3_t, emitter4_t> select_t;
#else // normal HETERO
    typedef qd::selector::Hetero_Emitter_pool< phen_t, Params, emitter1_t, emitter2_t, emitter3_t, emitter4_t> select_t;
#endif
    typedef qd::container::Grid_V2<phen_t, Params> container_t;

    
#else //map-elites
    std::cout<<"MAP-Elites"<<std::endl;
    typedef qd::selector::Uniform<phen_t, Params> select_t;
    typedef qd::container::Grid<phen_t, Params> container_t;
#endif

    


    
    typedef eval::Parallel<Params> eval_t;
    
    typedef boost::fusion::vector<
      stat::BestFit<phen_t, Params>, 
      //stat::QdContainer<phen_t, Params>,
#ifdef UCB
      stat::QdMAB<phen_t, Params>,
#endif
      //stat::QdPrintMap<phen_t,Params>, //This requires to have a visu_server running. I commented this out, to make sure that it does not cause troubles to newcomers. Feel free to uncomment it if you have a visu_server running (or any other Xsever running).
      stat::CMAProgress<phen_t, Params>,
      stat::DeleteGenFiles<phen_t, Params>
      >
      stat_t; 
    typedef modif::Dummy<> modifier_t;
#ifdef MAPELITES
    typedef qd::QualityDiversity<phen_t, eval_t, stat_t, modifier_t, select_t, container_t, Params> qd_t;
#else //CMAME or HETERO
    typedef qd::CMA_QD<phen_t, eval_t, stat_t, modifier_t, select_t, container_t, Params> qd_t;
#endif
    qd_t qd;
    run_ea(argc, argv, qd);
    //std::cout<<"best fitness:" << qd.stat<0>().best()->fit().value() << std::endl;
    //std::cout<<"archive size:" << qd.stat<1>().archive().size() << std::endl;
    return 0;
    
}




