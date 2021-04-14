#ifndef STAT_QD_CMA_PROGRESS_HPP_
#define STAT_QD_CMA_PROGRESS_HPP_

#include <numeric>
#include <sferes/stat/stat.hpp>

namespace sferes {
    namespace stat {
        SFERES_STAT(CMAProgress, Stat){
        public:
            template <typename E> void refresh(const E& ea)
            {
                if (ea.gen() % Params::pop::dump_period == 0)
                    _write_progress(std::string("progress"), ea);
            }
	    
            template <typename EA>
            void _write_progress(const std::string& prefix, const EA& ea) const
            {
                std::cout << "writing..." << prefix << std::endl;
                std::string fname = ea.res_dir() + "/" + prefix + std::string(".dat");
                std::ofstream ofs(fname.c_str(), std::ofstream::out | std::ofstream::app);

                size_t archive_size = ea.pop().size();
                double archive_max = this->scale(ea.pop()[0]->fit().value(),ea);
                double sum_novelty = 0.0f;
                double sum_quality = 0.0f;
                double var_novelty = 0.0f;

                for (auto it = ea.pop().begin(); it != ea.pop().end(); ++it) {
		  //assert((*it)->fit().value()>= - min);
		  sum_quality += this->scale((*it)->fit().value(),ea);
                    sum_novelty += (*it)->fit().novelty();
                    if (archive_max < this->scale((*it)->fit().value(),ea) )
		      archive_max = this->scale((*it)->fit().value(),ea);
                }

                for (auto it = ea.pop().begin(); it != ea.pop().end(); ++it) {
                    var_novelty += std::pow((*it)->fit().novelty() - sum_novelty / archive_size, 2);
                }
                var_novelty /= archive_size;

                ofs << ea.gen() << " " << archive_size << " "
                    << " " << archive_max << " " << sum_quality << "   " << sum_novelty << " "
                    << var_novelty << std::endl;
            }
	    template <typename EA>
	      double scale(double val, const EA& ea) const{
	      //we assume that 0 is the max.
#ifdef SPHERE
	      double min=(float)ea.pop()[0]->gen().size()*(1.4*5.12)*(1.4*5.12); // scaline taken from the CMA-ME paper
#elif HEXA_UNI
	      double min = 0;
	      return val;  // no scaling
#elif HEXA_OMNI
	      double min= M_PI/2;
#elif RARM
	      double min= 1;
#else // RASTRIGIN and RASTRIGIN_MULTI
	      double min=(float)ea.pop()[0]->gen().size()*((1.4*5.12)*(1.4*5.12)); // scaling taken from the CMA-ME paper
#endif
	      return std::min(1.0, std::max(0.0,(val + min)/min));
	    }
	    
        };

    } // namespace stat
} // namespace sferes

#endif
