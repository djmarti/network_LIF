# network_LIF
C code for a generic balanced network of leaky integrate-and-fire neurons, with sparse and random connections and synaptic dynamics. Most of the details about the network model can be found in the article ['Dynamics of Sparsely Connected Networks of Excitatory and Inhibitory Spiking Neurons'](http://link.springer.com/article/10.1023/A%3A1008925309027) by Nicolas Brunel and published in Journal of Computational Neuroscience May 2000, Volume 8, Issue 3, pp 183-208. The only difference with the network model described in the article is that the code incorporates dynamics for synaptic currents.

## Installation and compilation

```shell
git clone https://github.com/djmarti/network_LIF
cd network_LIF
make
```
You can use `scons` instead of `make` if you prefer.


## Setup
Just compile the code with either make or scons in the network_LIF folder
