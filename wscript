#! /usr/bin/env python

import sys
import os
import sferes
sys.path.insert(0, sys.path[0]+'/src/hexapod_omnidirectional/waf_tools/')


from waflib.Configure import conf

import dart
import corrade
import magnum
import magnum_integration
import magnum_plugins
import robot_dart

def options(opt):
    opt.load('dart')
    opt.load('corrade')
    opt.load('magnum')
    opt.load('magnum_integration')
    opt.load('magnum_plugins')
    opt.load('robot_dart')

@conf
def configure(conf):
    print('conf exp:')
    conf.load('dart')
    conf.load('corrade')
    conf.load('magnum')
    conf.load('magnum_integration')
    conf.load('magnum_plugins')
    conf.load('robot_dart')


    conf.check_dart()
    conf.check_corrade(components='Utility PluginManager', required=False)
    conf.env['magnum_dep_libs'] = 'MeshTools Primitives Shaders SceneGraph GlfwApplication'
    if conf.env['DEST_OS'] == 'darwin':
        conf.env['magnum_dep_libs'] += ' WindowlessCglApplication'
    else:
        conf.env['magnum_dep_libs'] += ' WindowlessGlxApplication'
    conf.check_magnum(components=conf.env['magnum_dep_libs'], required=False)
    conf.check_magnum_plugins(components='AssimpImporter', required=False)
    conf.check_magnum_integration(components='Dart', required=False)

    if len(conf.env.INCLUDES_MagnumIntegration) > 0:
        conf.get_env()['BUILD_MAGNUM'] = True
        conf.env['magnum_libs'] = magnum.get_magnum_dependency_libs(conf, conf.env['magnum_dep_libs']) + magnum_integration.get_magnum_integration_dependency_libs(conf, 'Dart')
    conf.check_robot_dart()
    
    print('done')
    
def build(bld):

    # Quick and dirty link to python C++
    bld.env.LIBPATH_PYTHON = '/usr/lib/x86_64-linux-gnu/'
    bld.env.LIB_PYTHON = [ 'python3.6m']
    bld.env.INCLUDES_PYTHON = '/usr/include/python3.6m'

    
    sferes.create_variants(bld,
                           source = 'src/main.cpp',
                           includes = '. ../../',
                           uselib = 'TBB BOOST EIGEN PTHREAD MPI KDTREE PYTHON DART ROBOTDART',
                           use = 'sferes2',
                           target = 'exp',
                           variants =
                           [
                            'HETERO HEXA_OMNI',
                            'HETERO HEXA_UNI', 
                            'HETERO RARM',     
                            'HETERO SPHERE',
                            'HETERO RASTRIGIN',
                            'HETERO RASTRIGIN_MULTI',

                            'HETERO UCB HEXA_OMNI',
                            'HETERO UCB HEXA_UNI', 
                            'HETERO UCB RARM',     
                            'HETERO UCB SPHERE', 
                            'HETERO UCB RASTRIGIN', 
                            'HETERO UCB RASTRIGIN_MULTI',

                            'CMAME RAND HEXA_OMNI',
                            'CMAME RAND HEXA_UNI', 
                            'CMAME RAND RARM',     
                            'CMAME RAND SPHERE',
                            'CMAME RAND RASTRIGIN',
                            'CMAME RAND RASTRIGIN_MULTI',

                            'CMAME OPT HEXA_OMNI',
                            'CMAME OPT HEXA_UNI', 
                            'CMAME OPT RARM',  
                            'CMAME OPT SPHERE',
                            'CMAME OPT RASTRIGIN',
                            'CMAME OPT RASTRIGIN_MULTI',

                            'CMAME IMP HEXA_OMNI',
                            'CMAME IMP HEXA_UNI', 
                            'CMAME IMP RARM', 
                            'CMAME IMP SPHERE',
                            'CMAME IMP RASTRIGIN',
                            'CMAME IMP RASTRIGIN_MULTI',

                            'CMAME RDW HEXA_OMNI',
                            'CMAME RDW HEXA_UNI', 
                            'CMAME RDW RARM', 
                            'CMAME RDW SPHERE',
                            'CMAME RDW RASTRIGIN',
                            'CMAME RDW RASTRIGIN_MULTI',
                           ]
    )


                            
