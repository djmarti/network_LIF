#ifndef _NETWORK_H
#define _NETWORK_H 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include "parameters.h"
#include "eprintf.h"

#define MAX_SPIKES_PER_DT 4000

struct TableNSpikes {
        /* The following vector is a sort of circular buffer that stores the number
         * of spikes emitted by a population at a particular time. The vector is used
         * to implement transmission delays. */
        int size;
        int *num_spikes; /* number of spikes emitted at a particular time */
        int **indices;      /* indices of the neurons that have emitted a spike */
        /* Number of positions between past and present */
        int lag;
        /* index pointing where we are now in time  */ 
        int i_curr; 
        /* index to the past (t-transmission_delay). This is not really
         * necessary but may improve performance (TODO check if this is true) */ 
        int i_delay;
};

struct Dynamic_Array {
        size_t n;
        size_t size;
        double *data;
};

struct Projection_array {
        size_t n;
        size_t size;
        int *data;
};

struct ConnectionSet {
        /* indices of neurons projecting to the neuron */
        int *id_innervations;
        /* indices of neurons innervated by the neuron */
        struct Projection_array id_projections;
};

struct Neuron {
        double V_m;              /* membrane potential (in mV) */
        int ref_state;           /* counter for refractoriness */
        double I_fast;                /* fast current */
        double I_slow;                /* slow current */
        /* data relative to fast and slow synapses. We use the same
         * connectivity matrix for both */
        struct ConnectionSet synapses;
        /* Dynamic array of the individual spike train */
        struct Dynamic_Array spike_train;
};

struct Network {
        int N;
        int NE; /* Number of excitatory cells */
        int NI; /* ... and of inhibitory cells */
        double g; /* g = |J_inh / J_exc| */
        double J; /* baseline efficacy of exc connections */
        int C;  /* Number of connections per neuron (fixed!) */
        int CE; /* Number of excitatory connections per neuron */
        int CI; /* ... and of inhibitory congections per neuron */
        double tau_m;
        double tau_slow; /* Slow synaptic time constant */
        double tau_fast; /* Slow synaptic time constant */
        bool slow_flag;
        double tau_rp; /* Refractory period in ms */
        int top_ref_state; /* Ref. period in timesteps */

        double delay; /* Transmission delay in ms */

        double ext_current;

        struct Neuron *cell;        /* Pointer to the array of neurons */

        /* Table of spikes */
        struct TableNSpikes tab_spikes;

        /* Counter of spikes within time window */
        int n_spikes;
};

/* network.c */
void setup_network(struct Network *ntw);
void setup_rng(void);
double gaussrand(void);
double uniform(void);
void sample_without_replacement(int N, int n, int exclude_i, int *v);
double exponential(void);
void allocate_synaptic_structures(struct Network *ntw);
void fill_synaptic_matrix(struct Network *ntw);
void initialize_table_of_spikes(struct Network *ntw, int lag);
void vidual_spike_train(struct Neuron *nrn);
void initialize_dynamic_array_projections(struct ConnectionSet *cnn, int Cbroad);
void initialize_individual_vars_for_neurons(struct Network *ntw);
double euler(struct Network *ntw, struct Neuron *nrn, double V);
void free_network(struct Network *ntw);
void free_rng(void);
void push_innervation(struct ConnectionSet *cnn, size_t i);
void push_spike(struct Neuron *nrn, double spike_time);
void save_pdfs_synaptic_vars(struct Network *ntw);
#endif
