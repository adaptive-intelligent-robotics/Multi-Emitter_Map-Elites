#ifndef LINE_VARIATION
#define LINE_VARIATION

#include <sferes/gen/evo_float.hpp>

namespace sferes {
  namespace gen {
    namespace evo_float {
      const size_t line = 3; //this is not super clean, but will do.
      
      // line crossover
      template<typename Ev>
      struct CrossOver_f<Ev, line>
      {
	void operator()(const Ev& f1, const Ev& f2, Ev &c1, Ev &c2)
	{
	  
	  SFERES_CONST float sigma_iso = Ev::params_t::evo_float::sigma_iso;
	  SFERES_CONST float sigma_line = Ev::params_t::evo_float::sigma_line;

	  
	  float sig2  = sigma_line  *  misc::gaussian_rand<float>(0,1); // sigma line for first offspring
	  float sig2p = sigma_line  *  misc::gaussian_rand<float>(0,1); // sigma line for second offspring
	  
	  //std::cout<<"sig2: "<<sig2 <<" sig2p "<<sig2p<<std::endl;
	  for (size_t i = 0; i < f1.size(); ++i)
	    {
	      float sig1  = sigma_iso * misc::gaussian_rand<float>(0,1); // sigma iso for first offspring
	      c1.data(i, misc::put_in_range(
					    f1.data(i) + sig1  + sig2  * (f2.data(i) - f1.data(i)),
					    0.0,
					    1.0));
	      
	      float sig1p = sigma_iso * misc::gaussian_rand<float>(0,1); // sigma iso for second offspring     
	      c2.data(i, misc::put_in_range(
					    f2.data(i) + sig1p + sig2p * (f1.data(i) - f2.data(i)),
					    0.0,
					    1.0));
		      
					    }

	  
	}
      };

    } //evo_float
  } // gen
} // sferes

#endif
