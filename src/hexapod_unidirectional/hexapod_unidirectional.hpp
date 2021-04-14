#ifndef ___AIRL_TASKS_HEXAPOD_UNIDIRECTIONAL_HPP__
#define ___AIRL_TASKS_HEXAPOD_UNIDIRECTIONAL_HPP__

#include <sferes/gen/evo_float.hpp>
#include <sferes/phen/parameters.hpp>
#include <sferes/fit/fit_qd.hpp>

#include <robot_dart/robot_dart_simu.hpp>
#include <robot_dart/control/hexa_control.hpp>



#include <dart/collision/bullet/BulletCollisionDetector.hpp>
#include <dart/constraint/ConstraintSolver.hpp>

#include "desc_hexa.hpp"


using namespace sferes::gen::evo_float;

namespace hexapod_unidirectional {
  
  
  // Definition of default parameter values
  struct Params_default {
    struct parameters {
      SFERES_CONST float min = 0;
      SFERES_CONST float max = 1;
    };
    struct evo_float {
      SFERES_CONST float cross_rate = 0.0f;
      SFERES_CONST float mutation_rate = 0.03f;
      SFERES_CONST float eta_m = 10.0f;
      SFERES_CONST float eta_c = 10.0f;
      SFERES_CONST mutation_t mutation_type = polynomial;
      SFERES_CONST cross_over_t cross_over_type = sbx;
    };

    struct nov {
        SFERES_CONST size_t deep = 3;
        SFERES_CONST double l = 0.01; 
        SFERES_CONST double k = 15; 
        SFERES_CONST double eps = 0.1;
    };

    struct qd {
      SFERES_CONST size_t behav_dim = 6;
      SFERES_ARRAY(size_t, grid_shape, 5,5,5,5,5,5);
    };
    
    
  };
  
  
  
  
  
  //initialising a global variable (but using a namespace) - this global variable is the robot object 
  namespace global{
    std::shared_ptr<robot_dart::Robot> global_robot; //initialize a shared pointer to the object Robot from robotdart and name it global_robot 
  }
  
  //Function to initialise the hexapod robot 
  void load_and_init_robot(std::string path)
  {
    std::cout<<"INIT Robot"<<std::endl;
    global::global_robot = std::make_shared<robot_dart::Robot>(path+"/hexapod_v2.urdf");
    global::global_robot->set_position_enforced(true);
    
    global::global_robot->set_actuator_types(dart::dynamics::Joint::SERVO);
    global::global_robot->skeleton()->enableSelfCollisionCheck();
    std::cout<<"End init Robot"<<std::endl; //Print End init Robot
  }
  
  
  
  FIT_QD(Hexapod_unidirectional)
  {
  public:
    Hexapod_unidirectional(){  }
  
    typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic, Eigen::RowMajor > Mat;

    
    template<typename Indiv>
      void eval(Indiv& ind)
    {

      std::vector<double> bd;
      std::tie(this->_value, bd, this->_dead) = simulate(ind.data());
      this->set_desc(bd);

    }



    // simulate function that receives the controller parameters and outputs the fitness, DB and potential death
    std::tuple<float, std::vector<double>, bool > simulate (const std::vector<float>& indiv) const
    {
      std::vector<double> ctrl(36);
      for(size_t i=0;i<indiv.size();i++)
	{
	  ctrl[i] = round( std::min(1.0f, std::max(0.0f,indiv[i])) * 1000.0 ) / 1000.0;// limite numerical issues
	}
      

    
      auto g_robot=global::global_robot->clone();
      g_robot->skeleton()->setPosition(5, 0.15); //original value 0.15
    
    
      double ctrl_dt = 0.02;
      g_robot->add_controller(std::make_shared<robot_dart::control::HexaControl>(ctrl_dt, ctrl));
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->set_h_params(std::vector<double>(1, ctrl_dt));
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->activate(false);
      robot_dart::RobotDARTSimu simu(ctrl_dt);
    
      simu.world()->getConstraintSolver()->setCollisionDetector(dart::collision::BulletCollisionDetector::create());
      simu.add_floor();
      simu.add_robot(g_robot);
    
    
    
      //stabilising the robot for 0.5 second at the 0 pos
      simu.run(0.5);
      simu.world()->setTime(0);
      //run the controller to go to the t=0 pos and stabilise for 0.5 second
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->activate(true);
      simu.world()->setTime(0);
      simu.run(0.05);
      simu.world()->setTime(0);
      simu.run(0.05);
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->activate(false);
      simu.run(0.5);
    
      simu.add_descriptor(std::make_shared<robot_dart::descriptor::DutyCycle>(robot_dart::descriptor::DutyCycle(simu)));
    
    
      // run the controller for 5 seconds
      simu.world()->setTime(0);
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->activate(true);
      simu.run(5.0);
    
      std::vector<double> results;    
      std::static_pointer_cast<robot_dart::descriptor::DutyCycle>(simu.descriptor(0))->get(results);
    
    
      // stops the controller and stabilize for 0.5 seconds.
      std::static_pointer_cast<robot_dart::control::HexaControl>(g_robot->controllers()[0])->activate(false);
      simu.world()->setTime(0);
      simu.run(0.5);
    
    
      auto pos=simu.robots().back()->skeleton()->getPositions().head(6).tail(3);
      g_robot.reset();
      
      return {pos[0], results, false};
    }
  
  
  
  
  
  private:
    std::vector<Eigen::VectorXd> _traj;
  
  
  };


    
  // Definition of the Genotype, Phenotype and Fitness object according to a provided Param Object.
  template<typename Params>
  using fit_t = Hexapod_unidirectional<Params>;
  template<typename Params>
  using gen_t = sferes::gen::EvoFloat<36, Params>;
  template<typename Params>
  using phen_t = sferes::phen::Parameters<gen_t<Params>, fit_t<Params>, Params>;

}

#endif
