#ifndef _SIMULATION_H
#define _SIMULATION_H 1

#define MAX_SUFFIX_LENGTH 70
#include <getopt.h>
#include <stdbool.h>
#include <gsl/gsl_histogram.h>
#include <gsl/gsl_sort.h>
#include "network.h"
#include "parameters.h"

struct Simulation {
    double time;
    double offset; /* offset after which we start considering spike times */
    double DT;
    double exp_decay_slow;
    double exp_decay_fast;
    double total_time;
    double time_window_size;
    char config_file[MAX_SUFFIX_LENGTH];
    char suffix[MAX_SUFFIX_LENGTH];
    FILE *spikes_file;
    FILE *pop_rates_file;
    FILE *indiv_rates_file;
    _Bool verbose;
};

struct State {
    struct Simulation sim;
    struct Network ntw;
};


/* simulation.c */
void setup_simulation(struct Simulation *sim);
void create_suffix(struct State *S, char *sfx);
void open_file_handlers(struct State *S);
void write_header(FILE* dev, struct State *S);
void setup_state(struct State *S);
void free_state(struct State *S);
void set_total_time(struct State *S, double T);
void set_offset(struct State *S, double offset);
void set_dt(struct State *S, double d);
void set_time_window_size(struct State *S, double w);
int initialize_network(struct State *S);
void free_simulation(struct Simulation *sim);
void simulate_one_step(struct State *S);
void update_membrane_potentials(struct State *S);
void send_away_spikes(struct State *S);
void update_pivots(struct State *S);
void fill_population_spike_train(struct State *S, struct Dynamic_Array *poptrain, int n_neurons);
void compute_average_autocorrelations(struct State *S);
void compute_global_autocorrelations(struct State *S);
void flush_population_rate(struct State *S);
double population_rate(struct State *S);
void reset(struct State *S);
void save_spike_activity(struct State *S);
void save_individual_firing_rates(struct State *S);
void show_parameters(struct State *S);
#endif
