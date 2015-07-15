#include "simulation.h"

void setup_simulation(struct Simulation *sim)
{
        /* Initialize SOME of the variables */
        /* (total_time is not set, for instance) */
        sim->time = 0.0;
        sim->offset = 0.0; 
        sim->DT = 0.05;
        sim->time_window_size = 1.0;
        sim->verbose = false;
        strcpy(sim->config_file, "brunel2000.conf");
}

void create_suffix(struct State *S, char *sfx)
{
        char times_str[28];
        char tau_fast_str[5];
        char delay_str[5];
        char *t1, *d1;
        sprintf(tau_fast_str, "%04.1f", S->ntw.tau_fast);
        sprintf(delay_str, "%4.2f", S->ntw.delay);
        t1 = string_replace(tau_fast_str, '.', 'p');
        d1 = string_replace(delay_str, '.', 'p');
        if (S->ntw.slow_flag)
                sprintf(times_str, "_delay_%s_T1_%s_T2_%d", d1, t1, (int) S->ntw.tau_slow);
        else
                sprintf(times_str, "_delay_%s_T1_%s", d1, t1);
        sprintf(S->sim.suffix, sfx);
        strcat(S->sim.suffix, times_str);
        strcat(S->sim.suffix, ".dat");
}

void open_file_handlers(struct State *S)
{
        char filename[100];
        char sfx[20];
        struct Simulation *sim = &S->sim;

        sprintf(sfx, "N%d_mu%02d", S->ntw.N, (int) (S->ntw.ext_current));
        create_suffix(S, sfx);

        sprintf(filename, "spikes_%s", S->sim.suffix);
        sim->spikes_file = fopen(filename, "w");
        sprintf(filename, "population_rate_%s", S->sim.suffix);
        sim->pop_rates_file = fopen(filename, "w");
}

void write_header(FILE* dev, struct State *S)
{
        struct Network *ntw = &S->ntw;
        fprintf(dev, "# N = %d, C = %d, J = %4.2f, mu = %4.2f, g = %3.1f, dt = %4.2f \n",
                        ntw->N, ntw->C, ntw->J, ntw->ext_current, ntw->g, S->sim.DT);
        fprintf(dev, "# tau_m = %4.1f, tau_rp = %5.3f, D = %5.3f, tau_fast = %5.3f",
                        ntw->tau_m, ntw->tau_rp, ntw->delay, ntw->tau_fast);
        if (S->ntw.slow_flag)
                fprintf(dev, ", tau_slow = %.1f\n#\n", ntw->tau_slow);
        else
                fprintf(dev, "\n#\n");
}

void setup_state(struct State *S)
{
        setup_network(&S->ntw);
        setup_simulation(&S->sim);
}

void free_state(struct State *S)
{
        free_network(&S->ntw);
        free_simulation(&S->sim);
}

void set_total_time(struct State *S, double T)
{
        S->sim.total_time = T;
}

void set_offset(struct State *S, double offset)
{
        S->sim.offset = offset;
}

void set_dt(struct State *S, double d)
{
        S->sim.DT = d;
}

void set_time_window_size(struct State *S, double w)
{
        S->sim.time_window_size = w;
}

int initialize_network(struct State *S)
{
        struct Network *ntw = &S->ntw;
        double dt = S->sim.DT;
        S->sim.exp_decay_fast = exp(-dt/ntw->tau_fast);
        S->sim.exp_decay_slow = exp(-dt/ntw->tau_slow);

        double rei = (double) ntw->NE / (double) ntw->NI;
        double f = rei / (1 + rei); 
        ntw->CI = (int) ((1.0 - f) * ntw->C + 0.5);
        ntw->CE = ntw->C - ntw->CI;

        /* Number of positions between past and present */
        int lag = (int) ceil(ntw->delay / dt);

        /* allocate memory for all neurons in the population */
        ntw->cell = emalloc(ntw->N * sizeof(struct Neuron));
        ntw->top_ref_state = (int) ntw->tau_rp / dt;
        initialize_table_of_spikes(ntw, lag);
        initialize_individual_vars_for_neurons(ntw);
        allocate_synaptic_structures(ntw);
        return 0;
}

void free_simulation(struct Simulation *sim) 
{
        fclose(sim->spikes_file);
        fclose(sim->pop_rates_file);
}

void simulate_one_step(struct State *S)
{
        struct Simulation *sim = &S->sim;
        update_membrane_potentials(S);
        send_away_spikes(S);
        update_pivots(S);
        sim->time += sim->DT;
}

void update_membrane_potentials (struct State *S)
{
        struct Simulation *sim = &S->sim;
        struct Network *ntw = &S->ntw;
        struct Neuron *nrn;

        double V_k;
        int i_curr;

        double dt = S->sim.DT;
        double interpolator;
        double spike_time;
        ntw->tab_spikes.num_spikes[ntw->tab_spikes.i_curr] = 0;

        for (int j = 0; j < ntw->N; j++) {
                nrn = &ntw->cell[j];
                if ( nrn->ref_state > 0 ) {
                        nrn->ref_state--;
                        continue;
                }
                V_k = nrn->V_m;
                nrn->V_m += dt * euler(ntw, nrn, V_k);
                i_curr = ntw->tab_spikes.i_curr;

                /* Threshold crossing ------------------------- */
                if ( nrn->V_m >= V_thr ) {
                        interpolator = (V_thr - V_k) / (nrn->V_m - V_k); /* This should be in [0,1] */
                        spike_time = sim->time + interpolator * dt;
                        ntw->tab_spikes.indices[i_curr][ntw->tab_spikes.num_spikes[i_curr]] = j;
                        if (ntw->tab_spikes.num_spikes[i_curr] >= MAX_SPIKES_PER_DT - 1) {
                                report("We have %d spikes in a time step, ", 
                                                ntw->tab_spikes.num_spikes[i_curr]);
                                report("which is a too big number\nfor the container ");
                                report("we use to store spike identities.\n Please, increase the");
                                report("value of MAX_SPIKES_PER_DT and recompile.\n");
                                exit (2);
                        }
                        ntw->tab_spikes.num_spikes[i_curr]++;
                        if (spike_time > S->sim.offset)
                                ntw->n_spikes++;
                        /* push_spike(nrn, spike_time); */
                        nrn->ref_state = ntw->top_ref_state;
                        nrn->V_m = V_reset 
                                + dt * euler(ntw, nrn, V_reset) * (1.0 - interpolator); 
                }
        }
        /* Update currents */
        for (int j = 0; j < ntw->N; j++) {
                nrn = &ntw->cell[j];
                nrn->I_fast *= S->sim.exp_decay_fast;
        }
        if (ntw->slow_flag)
                for (int j = 0; j < ntw->N; j++) {
                        nrn = &ntw->cell[j];
                        nrn->I_slow *= S->sim.exp_decay_slow;
                }
}

void send_away_spikes(struct State *S)
{
        struct Network *ntw = &S->ntw;
        struct Neuron *source; /* source neuron */
        struct Neuron *target; /* target neuron */
        int i_source, i_target, i_delay;
        double efficacy;
        int n_proj_cell;
        double scale_fast = (ntw->tau_m / ntw->tau_fast);
        double scale_slow = (ntw->tau_m / ntw->tau_slow);

        double JE = ntw->J;
        double JI = -ntw->g * ntw->J;

        i_delay = ntw->tab_spikes.i_delay;
        /* Loop over cells that emitted spikes at t-transmission_delay */
        for (int j = 0; j < ntw->tab_spikes.num_spikes[i_delay]; j++) {
                i_source = ntw->tab_spikes.indices[i_delay][j];
                source = &ntw->cell[i_source];
                if (i_source < ntw->NE)
                        efficacy = JE;
                else
                        efficacy = JI;
                n_proj_cell = source->synapses.id_projections.n;
                /* Loop over the projections for this cell */
                for (int m = 0; m < n_proj_cell; m++) {
                        i_target = source->synapses.id_projections.data[m];
                        target = &ntw->cell[i_target];
                        target->I_fast += efficacy * scale_fast;
                }
                if (ntw->slow_flag) {
                        for (int m = 0; m < n_proj_cell; m++) {
                                i_target = source->synapses.id_projections.data[m];
                                target = &ntw->cell[i_target];
                                target->I_slow += efficacy * scale_slow;
                        }
                }
        }
}

void update_pivots(struct State *S)
{
        struct Network *ntw = &S->ntw;
        ntw->tab_spikes.i_curr++;
        ntw->tab_spikes.i_delay++;
        /* Reset circular array if necessary */
        if (ntw->tab_spikes.i_curr == ntw->tab_spikes.size)
                ntw->tab_spikes.i_curr = 0;
        if (ntw->tab_spikes.i_delay == ntw->tab_spikes.size)
                ntw->tab_spikes.i_delay = 0;
}

void reset(struct State *S)
{
        S->sim.time = 0;
        S->ntw.n_spikes = 0;
        for (int i = 0; i < S->ntw.N; i++)
                S->ntw.cell[i].spike_train.n = 0;
        /* To carry over the spikes from the previous trial,
         * comment out the following loop. */
        struct TableNSpikes *t;
        t = &S->ntw.tab_spikes;
        for (int i = 0; i < t->size; i++)
                t->num_spikes[i] = 0;
}

void fill_population_spike_train(struct State *S, struct Dynamic_Array *poptrain, int n_neurons)
{
        /* Fill entries */
        struct Dynamic_Array *s;
        int id = 0;
        for (int i = 0; i < n_neurons; i++) {
                s = &S->ntw.cell[i].spike_train;
                for(size_t j = 0; j < s->n; j++)
                        if (s->data[j] > S->sim.offset) {
                                poptrain->data[id] = s->data[j];
                                id++;
                        }
        }
        poptrain->n = id;
}

void compute_average_autocorrelations(struct State *S)
/* Compute spike-time autocorrelation */
{
        struct Network *ntw = &S->ntw;
        struct Dynamic_Array *nrn_train;
        int n_neurons_sample = 1000;
        size_t num_spikes_total = 0;
        const size_t n_bins = 201;
        const double max_lag = 50; /* in ms */
        double bin_width = 2 * max_lag / (double) n_bins; 
        char filename[100];
        gsl_histogram *h = gsl_histogram_alloc(n_bins);
        gsl_histogram_set_ranges_uniform(h, -max_lag, max_lag);
        FILE *f;

        report("Computing average autocorrelation...\n");
        sprintf(filename, "autocorrelation_%s", S->sim.suffix);

        double t_sp;
        size_t l, r;
        f = fopen(filename, "w");
        for (int i = 0; i < n_neurons_sample; i++) { 
                l = 0;
                r = 0;
                nrn_train = &ntw->cell[i].spike_train;
                num_spikes_total += nrn_train->n;
                for (size_t j = 0; j < nrn_train->n; j++) {
                        t_sp = nrn_train->data[j];
                        /* look for left index */
                        while (l <  nrn_train->n && nrn_train->data[l] < t_sp - max_lag)
                                l++;
                        /* look for right index */
                        while (r < nrn_train->n && nrn_train->data[r] < t_sp + max_lag)
                                r++;
                        /* And feed the histogram with the relevant spikes  */
                        for (size_t k = l; k < r; k++) 
                                gsl_histogram_increment(h, t_sp - nrn_train->data[k]);
                }
        }
        /* gsl_histogram_fprintf(f, h, "%g", "%g"); */
        /* correct for boundary effects and substract mean */
        double w;
        double T = S->sim.total_time - S->sim.offset;
        /* double nu = num_spikes_total / (T * (double) n_neurons_sample); */
        double lower, upper, ac;
        int status;
        for (size_t j = 0; j < n_bins; j++) {
                w = T - fabs( (n_bins - 1.0) / 2.0 - j ) * bin_width;
                ac = gsl_histogram_get(h, j) / (w * n_neurons_sample);
                ac -= (pow(num_spikes_total / (T * n_neurons_sample), 2) * bin_width);
                /* ac /= pow(nu * bin_width, 2); [> Normalization <] */
                status = gsl_histogram_get_range(h, j, &lower, &upper);
                if (status==0) {
                        fprintf(f, "% 9.4f % 9.4f % 9.7f\n", lower, upper, ac);
                } else {
                        report("Something wrong here.\n");
                        exit(2);
                }
        }
        fclose(f);
        gsl_histogram_free(h);
}

void compute_global_autocorrelations(struct State *S)
/* Compute population rate autocorrelation */
{
        struct Network *ntw = &S->ntw;
        int n_neurons_sample = 100;
        const size_t n_bins = 201;
        const double max_lag = 100; /* maximal lag in ms */
        double bin_width = 2 * max_lag / (double) n_bins; /* 2*max_lag */
        char filename[100];
        gsl_histogram *h = gsl_histogram_alloc(n_bins);
        gsl_histogram_set_ranges_uniform(h, -max_lag, max_lag);
        FILE *f;

        struct Dynamic_Array poptrain;
        size_t num_spikes_total = 0;
        for (int i = 0; i < n_neurons_sample; i++)
                num_spikes_total += ntw->cell[i].spike_train.n;
        poptrain.data = emalloc(num_spikes_total * sizeof(double));
        poptrain.n = 0;
        poptrain.size = num_spikes_total;
           
        report("Computing global (population rate) autocorrelation...\n");
        fill_population_spike_train(S, &poptrain, n_neurons_sample);

        gsl_sort(poptrain.data, 1, num_spikes_total);
        sprintf(filename, "global_autocorrelation_%s", S->sim.suffix);

        double t_sp;
        f = fopen(filename, "w");
        size_t l = 0;
        size_t r = 0;
        for (size_t j = 0; j < poptrain.n; j++) {
                t_sp = poptrain.data[j];
                /* look for left index */
                while (l < poptrain.n && poptrain.data[l] < t_sp - max_lag)
                        l++;
                /* look for right index */
                while (r < poptrain.n && poptrain.data[r] < t_sp + max_lag)
                        r++;
                /* And feed the histogram with the relevant spikes  */
                for (size_t k = l; k < r; k++) 
                        gsl_histogram_increment(h, t_sp - poptrain.data[k]);
        }
        /* gsl_histogram_fprintf(f, h, "%g", "%g"); */
        /* correct for boundary effects and substract mean */
        double w;
        double T = S->sim.total_time - S->sim.offset;
        /* double nu = num_spikes_total / (T * (double) n_neurons_sample); */
        double lower, upper, ac;
        int status;
        for (size_t j = 0; j < n_bins; j++) {
                w = T - fabs( (n_bins - 1.0) / 2.0 - j ) * bin_width;
                ac = gsl_histogram_get(h, j) / (w * n_neurons_sample);
                ac -= (pow(num_spikes_total / (T * n_neurons_sample), 2) * bin_width);
                /* ac /= pow(nu * bin_width, 2); [> Normalization <] */
                status = gsl_histogram_get_range(h, j, &lower, &upper);
                if (status==0) {
                        fprintf(f, "% 9.4f % 9.4f % 9.6f\n", lower, upper, ac);
                } else {
                        report("Somehting wrong here.\n");
                        exit(2);
                }
        }
        fclose(f);
        free(poptrain.data);
        gsl_histogram_free(h);
}

void flush_population_rate(struct State *S)
{
        struct Simulation *sim = &S->sim;
        struct Network *ntw = &S->ntw;
        double tmp;
        if ( (int) rint(sim->time / sim->DT) % 100 == 0) {
                report("% 9.3f   \r ", sim->time);  
                fflush(stdout);
        }
        fprintf(sim->pop_rates_file, "% 9.3f  ", sim->time);
        tmp = ntw->n_spikes / (double) (sim->time_window_size * ntw->N);
        fprintf(sim->pop_rates_file, "% 9.3f\n", 1e3 * tmp);  /* Rates in Hz */
        ntw->n_spikes = 0;
}

double population_rate(struct State *S)
{
        return 1e3 * S->ntw.n_spikes / (double) (S->ntw.N * (S->sim.total_time - S->sim.offset));
}

void save_spike_activity(struct State *S)
{
    struct Dynamic_Array *s;
    for (int i = 0; i < 100; i++) {
            s = &S->ntw.cell[i].spike_train;
            for (size_t k = 0; k < s->n; k++)
                    fprintf(S->sim.spikes_file, "% 7.3f % 4d\n", s->data[k], i);
    }
}

void save_individual_firing_rates(struct State *S)
{
        struct Simulation *sim = &S->sim;
        struct Network *ntw = &S->ntw;
        for (int i = 0; i < ntw->N; i++) {
                fprintf(sim->indiv_rates_file, "% 2d % 8.2f\n", i, 
                                1e3 * (ntw->cell[i].spike_train.n) / sim->time);
        }
}

void show_parameters(struct State *S)
{
        struct Simulation *sim = &S->sim;
        struct Network *ntw = &S->ntw;

        printf("\nConfig loaded from '%s':\n\n", sim->config_file);
        printf("   Network parameters\n");
        printf("       N, number of neurons       = % 6d\n", ntw->N);
        printf("       C, number of connections   = % 6d\n", ntw->C);
        printf("       f, fraction of exc cells   = % 6.2f\n", (double)ntw->NE / (double)ntw->N);
        printf("       T, membrane time constant  = % 6.2f\n", ntw->tau_m);
        printf("       r, refractory period       = % 6.2f\n", ntw->tau_rp);
        printf("       J, synaptic efficacy       = % 6.2f\n", ntw->J);
        printf("       g, |JI| / |JE|             = % 6.2f\n", ntw->g);
        printf("       t, synaptic time constant  = % 6.2f\n", ntw->tau_fast);
        if (ntw->slow_flag)
                printf("       s, slow synaptic time constant  = % 6.2f\n", ntw->tau_slow);
        printf("       D, synaptic delay          = % 6.2f\n", ntw->delay);
        printf("       I, external input          = % 6.2f\n\n", ntw->ext_current);
        printf("   Simulation parameters\n");
        printf("       Time step                  = % 6.2f\n", sim->DT);
        printf("       Total simulated time       = % 6d\n", (int)sim->total_time);
}

