import os
import sys
import glob
# Pandas for managing datasets
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import imageio
from natsort import natsorted, ns
import numpy as np
import re
import scipy
from seaborn.relational import _LinePlotter

exp_names =  ["hexa_uni", "hexa_omni", "rarm","sphere", "rastrigin", "rastrigin_multi" ]

variant_names = ["cmame_opt", "cmame_rdw", "cmame_imp", "cmame_rand", "hetero","hetero_ucb"]
#variant_names = ["hetero_mab"]


def get_config(exp):
    if exp == "hexa_uni":
        resolutions=[5,5,5,5,5,5]
        vmin = 0
        vmax = 2
    elif exp == "hexa_omni":
        resolutions=[100,100]
        vmin = -np.pi/2
        vmax = 0
    elif exp == "rarm":
        resolutions=[100,100]
        vmin = -1
        vmax = 0
    elif exp == "rastrigin":
        resolutions=[100,100]
        vmin = -100*((5.12*1.4)*(5.12*1.4))
        vmax = 0
    elif exp == "rastrigin_multi":
        resolutions=[100,100]
        vmin = -100*((5.12*1.4)*(5.12*1.4))  
        vmax = 0
    elif exp == "sphere":
        resolutions=[100,100]
        vmin = - 100*(5.12*1.4)*(5.12*1.4)
        vmax = 0

    return [vmin, vmax, resolutions]



def plot_archive_distribution(data):
    
    sns.set(style="white", rc={"axes.facecolor": (0, 0, 0, 0)})
    for exp in exp_names:
    
        plt.figure()
        
        # Initialize the FacetGrid object
        pal = sns.cubehelix_palette(10, rot=-.25, light=.7)
        g = sns.FacetGrid(data[data['exp']==exp], row="variant", hue="variant", aspect=15, height=.5, palette=pal)

        # Draw the densities in a few steps
        g.map(sns.kdeplot, "fit", clip_on=False, shade=True, alpha=1, lw=1.5, bw=.2)
        g.map(sns.kdeplot, "fit", clip_on=False, color="w", lw=2, bw=.2)
        plt.xlim(0, 1)
        g.map(plt.axhline, y=0, lw=2, clip_on=False)
    
    
        # Define and use a simple function to label the plot in axes coordinates
        def label(x, color, label):
            ax = plt.gca()
            ax.text(0, .2, label, fontweight="bold", color=color,
                    ha="left", va="center", transform=ax.transAxes)

        
        g.map(label, "fit")

        # Set the subplots to overlap
        g.fig.subplots_adjust(hspace=-.25)
    
        # Remove axes details that don't play well with overlap
        g.set_titles("")
        g.set(yticks=[])
        g.despine(bottom=True, left=True)
        png_name = "./ridge_"+exp+".png"
        plt.savefig(png_name)
               
        plt.close()

def load_archive_text_file(path, resolutions):
    archive = np.genfromtxt(path, usecols = range(1,2+len(resolutions)))

    for i in range(archive.shape[1]-1):
        archive[:,i] = np.around(archive[:,i]*(resolutions[i]-1))

    if(len(resolutions)>2):
        for x in range(0,2):
            space = resolutions[x]
            for i in range(x+2, len(resolutions), 2):
                archive[:,x] += archive[:,i]*space
                space *= resolutions[i];
        archive = archive[:,np.array([0,1, -1])]

    return archive


        
def load_archive(path, resolutions = [100,100]):
    archive = load_archive_text_file(path, resolutions)
    grid = np.empty((np.prod(resolutions[0::2]),np.prod(resolutions[1::2])))
    grid[:] = np.nan
    rows = np.array([archive[:,0]], dtype=np.intp)
    columns = np.array([archive[:,1]], dtype=np.intp)
    grid[rows,columns] = archive[:,2]
    
    return grid
        
def print_archive(arg):
    for exp in exp_names:
        for variant in variant_names:
            folders = get_files(arg,variant,exp, "")
            if(len(folders)==0):
                print("NO folder for "+exp+" "+variant)
                continue

            folder = folders[-1]
            files = glob.glob(folder+"/archive_*.dat")
            print(files)
            if(len(files)==0):
                print("NO file for "+exp+" "+variant)
                continue

            files = natsorted(files, alg=ns.IGNORECASE)
            images = []
            for path in files:
                config = get_config(exp)
                data = load_archive(path, config[2])

                fig=plt.figure()
                ax = fig.add_subplot(111, aspect=1)
                c = ax.pcolor(data,vmin=config[0], vmax=config[1], edgecolors = None, antialiaseds=True)
                fig.colorbar(c, ax=ax)
                plt.title(variant+"_"+exp+"_"+os.path.basename(path))

                png_name = "./"+variant+"_"+exp+"_"+os.path.basename(path)+".png"
                plt.savefig(png_name)
                plt.close()
                images.append(imageio.imread(png_name))
                os.remove(png_name) 
            imageio.mimsave("./"+variant+"_"+exp+"_archives.gif", images, fps = 2)
            
         
            
            


def plot_proportions(arg):
    data = collect_data(arg,"mab_proportion.dat",["gen","p1","p2", "p3", "p4","rp1","rp2", "rp3", "rp4"],True)
    print(data)
    sns.set(style="whitegrid")
    # Plot the responses for different events and regions
    for exp in exp_names:
        plt.figure()
        sns.palplot(sns.color_palette("colorblind"))
        f = plt.figure(figsize=(6.4, 4.8))
        ax = f.add_subplot(111)        
        for item in ["p1","p2","p3","p4"]: #,"rp1","rp2", "rp3", "rp4"]:
            sns_plot = sns.lineplot(x="gen", y=item,
                                    style="variant", 
                                    data=data[data['exp']==exp])
        
        plt.title(exp)
        ax.legend( labels=["OPT","IMP","RDW","RAND","reset_OPT","reset_IMP","reset_RDW","reset_RAND"])
        plt.savefig("./mab_progress_"+exp+".svg")
        
def plot_progress(arg):
    data = collect_data(arg)
    def first_second_third_quartile(self, vals, grouper, units=None):
        # Group and get the aggregation estimate
        grouped = vals.groupby(grouper, sort=self.sort)
        est = grouped.agg('median')
        min_val = grouped.quantile(0.25)
        max_val = grouped.quantile(0.75)
        cis = pd.DataFrame(np.c_[min_val, max_val],
                           index=est.index,
                           columns=["low", "high"]).stack()
        
        # Unpack the CIs into "wide" format for plotting
        if cis.notnull().any():
            cis = cis.unstack().reindex(est.index)
        else:
            cis = None
            
        return est.index, est, cis
    
    sns.set(style="whitegrid")
    sns.palplot(sns.color_palette("colorblind"))
    # Plot the responses for different events and regions
    for exp in exp_names:
        for item in ["archive_size","best_fit", "sum_fit"]:
            plt.figure()
            my_lineplot=sns.lineplot
            _LinePlotter.aggregate = first_second_third_quartile
            sns_plot = sns.lineplot(x="gen", y=item,
                                   hue="variant", 
                                   data=data[data['exp']==exp] )
            plt.title(exp+"_"+item)
            plt.savefig("./progress_"+exp+"_"+item+".svg")

            #sns_plot.set(xlim=(0, 100))
            #plt.savefig("./progress_"+exp+"_"+item+"_short.png")
            #plt.close()

    val = pd.DataFrame(columns=['exp','variant','median'])
    for exp in exp_names:
        pp = pd.DataFrame(columns=['ref_var','comp_var','p_value'])

        for ref_var in variant_names:
            med = np.median(data[(data.exp==exp) & (data.variant==ref_var)  & (data.gen==20000) ].sum_fit.to_numpy())
            val=val.append({'exp':exp, 'variant':ref_var, 'median': med}, ignore_index=True)
            for comp_var in variant_names:
                stat, p = scipy.stats.ranksums(
                    data[(data.exp==exp) & (data.variant==ref_var)  & (data.gen==20000) ].sum_fit.to_numpy(),
                    data[(data.exp==exp) & (data.variant==comp_var) & (data.gen==20000) ].sum_fit.to_numpy())
            
                #if(ref_var != comp_var):
                pp=pp.append({'ref_var':ref_var, 'comp_var':comp_var, 'p_value':p}, ignore_index=True)

        pp=pp.pivot(index='ref_var', columns='comp_var', values='p_value')
        f = open("pvalue_" + exp + ".md", "a")
        f.write(pp.to_markdown())
        f.close()
    val=val.pivot(index='exp', columns='variant',values='median')
    ff = open("medians.md", "a")
    ff.write(val.to_markdown())
    ff.close()
    

def get_files(arg, variant, exp, filename=""):
    if(len(arg)!=0):
        base = arg[0]
    else:
        base = "."


    if filename != "":
        filename ="/"+filename

    #if(exp == "hexa_uni" or exp == "hexa_omni"):
    #    return glob.glob(base+'/'+variant+'_'+exp+'/results_' + variant+"_"+exp+'/2020*'+filename)
    #else:
    #    return glob.glob(base+'/results_' + variant+"_"+exp+'/2020*' + filename)
    return glob.glob(base+'/results_' + variant+"_"+exp+'/202*/' + filename)


def collect_data(arg, filename = "progress.dat", fields = ["gen","archive_size","best_fit", "sum_fit", "div1", "div2"], rolling=False):

    data = pd.DataFrame()
    for exp in exp_names:
        for variant in variant_names:
            files = get_files(arg,variant,exp, filename)
            print(files)
            if(len(files)==0):
                print("NO file called " + filename + " for "+exp+" "+variant)
                continue
            data_tmp = pd.DataFrame()
            for run, f in enumerate(files): 
                tmp = pd.read_csv(f,delim_whitespace=True,names=fields)
                tmp['run']=run
                if( rolling):
                    for item in fields:
                        tmp[item] =tmp[item].rolling(50, win_type='triang').mean()
                    tmp = tmp[50::50]
                data_tmp=pd.concat([data_tmp, tmp], ignore_index=True)
                
            data_tmp['variant'] = variant
            data_tmp['exp'] = exp
            data = data.append(data_tmp)

    return data

    
def report_integrity(arg):
    data = collect_data(arg)
    data = data[['gen', 'run', 'variant','exp']]

    data = data.groupby(['exp','variant','run']).max()
    data = pd.pivot_table(data,values=['gen'],index=['exp','run'],columns=['variant'])
    print(data)
    S = data.to_markdown()
    #remove some artifacts
    S = re.sub("((\('gen'. ')|('\)))","",S) 
    S = re.sub("(\('.+(?P<id>[1-9]|\d{2,})\))","|\g<2>",S)
    S =	re.sub("(\('|\))","",S)
    S =	re.sub("(',\s)","|",S)
    S =	re.sub("(\|:---)","|:---|:---",S)
    S = "|   "+S

    f = open("integrity.md", "a")
    f.write(S)
    f.close()
    
    print(S)


        
if __name__ == "__main__":
    report_integrity(sys.argv[1:])
    #print_archive(sys.argv[1:])
    plot_progress(sys.argv[1:])
    plot_proportions(sys.argv[1:])   

    sys.exit(0)
    
