
# Multi-Emitter MAP-Elites: Improving quality, diversity and data efficiency with heterogeneous sets of emitters



This repository contains the code associated with [Multi-Emitter MAP-Elites: Improving quality, diversity and data efficiency with heterogeneous sets of emitters](https://arxiv.org/abs/2007.05352)

# Libraries and dependencies

The implementation of all tasks and algorithms is based on the qd-branch of the C++ [Sferes2](https://github.com/sferes2/sferes2)  library presented in [Sferesv2: Evolvin' in the multi-core world](https://ieeexplore.ieee.org/abstract/document/5586158/?casa_token=EhBJLkircvMAAAAA:ls8I90Y5H2vsJk5RxCYs8X1T9yZHDhDEz5S6g5gatOzETle1LK_ib8zwodx6t5J_-Uwq_YP9), and the hexapod  control task uses the Dart simulator introduced in [Dart: Dynamic animation and robotics toolkit](https://joss.theoj.org/papers/10.21105/joss.00500.pdf).
Furthermore, the analysis of the results is based on [Panda](https://pandas.pydata.org/), [Matplotlib](https://matplotlib.org/) and [Seaborn](https://seaborn.pydata.org/index.html) libraries.

# Structure

The main part of the code contains the following files and folders:
- The `src` folder contains the main structure the code based on the framework of QD algorithms introduced in [Quality and diversity optimization: A  unifying modular  framework](https://ieeexplore.ieee.org/abstract/document/7959075/). 
- The `python` folder is used for the results analysis.
- The `singularity` folder contains the required files to compile the Singularity container for this experiment.

# Execution

The results of the paper can be reproduced by running the Singularity container image of the experiment. Instructions to install Singularity can be found in [Singularity documentation](https://sylabs.io/guides/3.5/user-guide/quick_start.html#quick-installation-steps).
To compile the container, run the command: `python build_final_image.py` from the singularity folder.
The resulting container should be called something like `final_Multi-Emitter_Map-Elites_YYYY-MM-DD_HH_MM_SS.sif`

This container is an executable that can be used to launch an experiment by providing the name of the experiment as argument to the container. The argument should have the form of: `[algo]_[task]`. A valid example is `hetero_ucb_hexa_omni`

`algo` can take one of the following values
- `hetero_ucb`: Multi-Emitter MAP-Elites with UCB1.
- `hetero`: Multi-Emitter MAP-Elites with uniform sampling.
- `cmame_imp`: CMA-ME with Improving Emitter.
- `cmame_opt`: CMA-ME with Optimising Emitter.
- `cmame_rdw`: CMA-ME with Random Direction Emitter.
- `cmame_rand`: CMA-ME with Random Emitter (equivalent to MAP-Elites with line variation operator).

`task` can take one of the following values
- `rastrigin`: Noise-free Baseline for each of the three tasks.
- `rastrigin_multi`: Explicit-averaging approach with 1 re-sampling.
- `sphere`: Explicit-averaging approach with 50 re-sampling.
- `rarm`: DG-MAP-Elites approach with depth 50.
- `hexa_omni`: Adaptive sampling approach without drifting elites.
- `hexa_uni`: Adaptive sampling approach with drifting elites.




