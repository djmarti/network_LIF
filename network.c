/* Network of integrate-and-fire  neurons, with sparse connectivity
 * and simple synaptic dynamics 
 *
 *  Dani Martí. Jun 2013  */
#include "network.h"

gsl_rng *Rng;               /* Global random number generator */
const gsl_rng_type *Rng_T;  /* Type of rng */

void setup_network(struct Network *ntw)
{
        ntw->cell = NULL;
        ntw->ne_spikes = 0;
        ntw->ni_spikes = 0;
        ntw->slow_flag = false;
}


void setup_rng(void)
{
        /* Set up random generator */
        gsl_rng_env_setup();
        Rng_T = gsl_rng_default;
        Rng = gsl_rng_alloc(Rng_T);
}

/* wrapper to return a standard normal variate */
double gaussrand()
{
        return gsl_ran_gaussian(Rng, 1.0);
}

/* wrapper to return a uniform variate */
double uniform()
{
        return gsl_rng_uniform(Rng);
}

void swap(int *a, int *b)
{
        int tmp = *a;
        *a = *b;
        *b = tmp;
}

void shuffle(int *v, int n)
{
        double U;
        int k;
        int j = n - 1;

        while (j > 0) {
                U = uniform();
                k = floor(j * U);
                swap(&v[k], &v[j]);
                j--;
        }
}

void sample_without_replacement(int N, int n, int exclude_i, int *v)
{
        /* Selection sampling (Knuth 3.4.2). The third argument is the
         * index we want to exclude. This is useful if we want to avoid
         * autapses. The last argument is a pointer to the array of ints
         * where we want to store the indices. */
        int t = 0; /* total number of input records dealt with*/
        int m = 0; /* number of records selected so far */

        while (m < n) {
                if ((N - t) * uniform() >= n - m || t == exclude_i)
                        t++;
                else {
                        v[m] = t;
                        t++;
                        m++;
                }
        }
}

double exponential()
{
        /* Returns an exponential variate with rate 1.
         * For an arbitrary rate r, multiply the output by 1 / r */
        return -log(uniform());
}

void allocate_synaptic_structures(struct Network *ntw)
{
        struct Neuron *nrn;
        double epsilon = (double) ntw->C / (double) ntw->N;
        int dC = (int) sqrt(ntw->C * (1 - epsilon)) / 2;

        for (int i = 0; i < ntw->N; i++) {
                nrn = &ntw->cell[i];
                nrn->synapses.id_innervations = emalloc(ntw->C * sizeof(int));
                initialize_dynamic_array_projections(&nrn->synapses, ntw->C + 4 * dC);
        }
}


void fill_synaptic_matrix(struct Network *ntw)
{
        /* Data structures were already created in allocate_synaptic_structures.
         * Here we only reset counters and generate random indices. */
        struct Neuron *nrn;
        int pre_neuron;

        int C_exc = rint(ntw->C * ((double) ntw->NE / (double) ntw->N));
        int C_inh = rint(ntw->C * ((double) ntw->NI / (double) ntw->N));

        /* report("\n  Generating synaptic matrix... "); */
        /* fflush(stdout); */

        /* Reset projection counter. Remember that id_projections is a dynamic
         * array, so we don't need to do anything else. */
        for (int i = 0; i < ntw->N; i++)
                ntw->cell[i].synapses.id_projections.n = 0; 

        for (size_t i = 0; i < (size_t)ntw->N; i++) {
                nrn = &ntw->cell[i];
                /* For the nrn->synapses.id_innervations, we just replace the C
                 * random indices for each neuron. */
                /* Of the C innervations, C * f are excitatory */
                sample_without_replacement(ntw->NE, C_exc, i, nrn->synapses.id_innervations);
                /* ... and C * (1 - f) are inhibitory  */
                sample_without_replacement(ntw->NI, C_inh, i - ntw->NE, nrn->synapses.id_innervations + C_exc);
                /* add the offset for inhibitory neurons */
                for (int j = 0; j < C_inh; j++)
                        nrn->synapses.id_innervations[C_exc + j] += ntw->NE;
                /* Sesame street: if i is innervated by j, then j projects to
                 * i. We need to build the list of projections for each
                 * neuron---going reverse */
                for (int j = 0; j < ntw->C; j++) { 
                        pre_neuron = nrn->synapses.id_innervations[j];
                        push_innervation(&ntw->cell[pre_neuron].synapses, i);
                }
        }
}

void initialize_table_of_spikes (struct Network *ntw, int lag)
{
        struct TableNSpikes *t;
        t = &ntw->tab_spikes;
        t->i_delay = 0;  /* 0-th for the past t-delay*/
        t->i_curr = lag; /* the current time is a few indices ahead */
        t->lag = lag;
        t->size = lag + 1; /* the  size of the buffer */
        /* spikes emitted at each time slot */
        t->num_spikes = emalloc(t->size * sizeof(int)); 
        /* indices of the neurons that emitted spikes at a particular time slot */
        t->indices = emalloc(t->size * sizeof(int*));
        for (int i = 0; i < t->size; i++) {
                t->num_spikes[i] = 0;
                t->indices[i] = emalloc(MAX_SPIKES_PER_DT  * sizeof(int));
        }
}

void initialize_individual_spike_train(struct Neuron *nrn)
{
        struct Dynamic_Array *a;
        a = &nrn->spike_train;
        a->n = 0;
        a->size = 1000;    /* educated guess 100 Hz per neuron and 10 s of simulation*/
        a->data = emalloc(a->size * sizeof(double));
}

void initialize_dynamic_array_projections(struct ConnectionSet *cnn, int Cbroad)
{
        struct Projection_array *a;
        a = &cnn->id_projections;
        a->n = 0;
        a->size = Cbroad;
        a->data = emalloc(a->size * sizeof(int));
}

void initialize_individual_vars_for_neurons(struct Network *ntw)
{
        struct Neuron *nrn;
        /* We initialize the current assuming that nu_0 = 10Hz */
        double stdI = ntw->J * sqrt(ntw->CE * ntw->tau_m * 0.01 * (1 + pow(ntw->g, 2) * 0.8));

        for (int i = 0; i < ntw->N; i++) {
                nrn = & ntw->cell[i];
                if (uniform() < 0.2) {
                        nrn->ref_state = (int) ntw->top_ref_state * uniform();
                        nrn->V_m = V_reset;
                } else {
                        nrn->ref_state = 0;
                        /* Distribute uniformly between reset and threshold potentials */
                        nrn->V_m = V_reset + (V_thr - V_reset) * uniform();
                }
                nrn->I_fast = stdI * gaussrand();
                if (ntw->slow_flag)
                        nrn->I_slow = stdI * gaussrand();
                else
                        nrn->I_slow = 0;
                initialize_individual_spike_train(nrn);
        }
}

double euler (struct Network *ntw, struct Neuron *nrn, double V)
{
        return (-V  + ntw->ext_current + nrn->I_fast + nrn->I_slow) / ntw->tau_m;
}

void free_network(struct Network *ntw)
{
        for (int i = 0; i < ntw->N; i++) {
                free(ntw->cell[i].synapses.id_innervations);
                free(ntw->cell[i].synapses.id_projections.data);
                free(ntw->cell[i].spike_train.data);
        }
        /* Free the table of spikes */
        free(ntw->tab_spikes.num_spikes);
        for (int i = 0; i < ntw->tab_spikes.size; i++) {
                free(ntw->tab_spikes.indices[i]);
        }
        free(ntw->tab_spikes.indices);

        /* Free the array of neurons and contents */
        free(ntw->cell);
}

void free_rng(void) 
{
        gsl_rng_free(Rng);
}


void push_innervation(struct ConnectionSet *cnn, size_t i)
{
        /* neuron i is innervated by j, therefore j projects to i */
        struct Projection_array *a;
        a = &cnn->id_projections;
        int *b;

        if(a->n >= a->size) { /* grow */
                b = erealloc(a->data, (a->size + 1000) * sizeof(size_t));
                a->size += 1000;
                a->data = b;
        }
        a->data[a->n] = i;
        a->n++;
}

void push_spike(struct Neuron *nrn, double spike_time)
{
        struct Dynamic_Array *a;
        double *b;

        a = &nrn->spike_train;

        if(a->n >= a->size) { /* grow */
                b = erealloc(a->data, 2 * a->size * sizeof(double));
                a->size *= 2;
                a->data = b;
        }
        a->data[a->n] = spike_time;
        a->n++;
}

void save_pdfs_synaptic_vars(struct Network *ntw)
{
        struct Neuron *nrn;
        FILE *f;

        f = fopen("synaptic_variables_at_end.dat", "w");

        for (int i = 0; i < ntw->N; i++) {
                nrn = &ntw->cell[i];
                fprintf(f, "% 8.4e % 8.5e % 8.5e\n", 
                                nrn->V_m, nrn->I_fast, nrn->I_slow);
        }
        fclose(f);
}
