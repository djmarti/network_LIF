# network_LIF
C code for a generic balanced network of leaky integrate-and-fire neurons, with sparse and random connections and synaptic dynamics. Most of the details about the network model can be found in the article ['Dynamics of Sparsely Connected Networks of Excitatory and Inhibitory Spiking Neurons'](http://link.springer.com/article/10.1023/A%3A1008925309027) by Nicolas Brunel and published in Journal of Computational Neuroscience May 2000, Volume 8, Issue 3, pp 183-208. The only difference with the network model described in the article is that the code incorporates dynamics for synaptic currents.

The code tries to simple, clean, and readable, but it is far from being bug-free.

## Installation and compilation
Just get the code and compile the code with either `make` or [`scons`](http://www.scons.org) in the `network_LIF` folder:
```shell
git clone https://github.com/djmarti/network_LIF
cd network_LIF
make
```

## Running the code and measuring network activity
After compilation you will get an executable file called `simulate_one_trial`. This program 
simulates the activity of the network for 20 seconds and saves in text files different measures of activity:
* The spike activity of the first 100 neurons of the network... , saved under a file like `spikes_N_10000_mu_24_delay_0p50_T1_0p5_T2_100.dat`. This long filename contains information about the specific parameters used in the simulation. See [filename suffixes](#suffixes) below for more details. The file contains all the spikes emitted during the simulation, with each line containing the time when a spike was emitted (first column) and the identifier of the neuron that emitted the spike (second column).
* The population activity of the excitatory and inhibitory populations, measured on non-overlapping sliding windows of width 0.5 ms.
* The average spike-train autocorrelation. 
* The autocorrelation of the population activities (excitatory and inhibitory). 


## Specifying parameters
You can specify the values of different network parameters with the command line and a configuration file. The configuration file is called `brunel2000.conf' and sets the default values. The command line options can be used to override the default values without having to edit the config file. 

```
Usage: ./simulate_one_trial [OPTIONS]

Simulate a network of spiking neurons and save spike activity,
population rates, and autocorrelations.

  General settings:
    -c, --config-file=FILE              read configuration parameters from FILE
    -v, --verbose                       be verbose.

  Network parameters:
    -N, --number-of-neurons=INT         set the total number of neurons
    -C, --number-of-connections=INT     set the number of connections per cell
    -f, --fraction-excitatory=REAL      set the fraction of excitatory cells in the whole network
    -J, --synaptic-efficacy=REAL        set the synaptic efficacy of excitatory connections
    -g, --inh-to-exc-weight-ratio=REAL  set the ratio between inhibitory to excitatory efficacies (absolute value)
    -T, --membrane-time-constant=REAL   set the membrane time constant (in ms)
    -r, --refractory-period=REAL        set the refractory period (in ms) 
    -D, --synaptic-delay=REAL           set the synaptic delay (in ms) 
    -t, --synaptic-time-constant=REAL   set the time constant of fast synapses
    -I, --external-current=REAL         set the homogeneous external current (in mV)

  Miscellaneous:
    -h, --help                          display this help and exit
```
