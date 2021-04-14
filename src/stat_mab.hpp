#ifndef STAT_QD_MAB_HPP_
#define STAT_QD_MAB_HPP_

#include <numeric>
#include <sferes/stat/stat.hpp>
#include "matplotlibcpp.h"
#include <cmath>

namespace plt = matplotlibcpp;

namespace sferes {
    namespace stat {
        SFERES_STAT(QdMAB, Stat){
        public:
            template <typename E> void refresh(const E& ea)
            {
	      assert(!ea.pop().empty());
	      
	      this->_create_log_file(ea, "mab_proportion.dat");
	      if (ea.dump_enabled()){
		const auto prop=ea.selector().get_proportions();
		(*this->_log_file) << ea.gen() << " " ;
		for(auto p : prop)
		  (*this->_log_file) << p << " ";

		const auto reset_prop=ea.selector().get_reset_prop();
		for(auto p : reset_prop)
		  (*this->_log_file) << p << " ";

		(*this->_log_file) << std::endl;
	      }
	    }
  
        };

    } // namespace stat
} // namespace sferes

#endif
