#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "parser.h"
#include "network.h"
#include "simulation.h"
#include "eprintf.h"

int main(int argc, char *argv[])
{
    int status = 0;
    struct State S;
    setup_state(&S);
    setup_rng();
    set_total_time(&S, 20000); /* duration simulation (ms) */
    set_dt(&S, 0.05); /* timestep (ms) */
    /* width of the window over which we sample population rates */
    set_time_window_size(&S, 0.5);
    status = read_network_parameters(argc, argv, &S);
    status = initialize_network(&S);
    fill_synaptic_matrix(&S.ntw);
    open_file_handlers(&S);

    int n_skipped_samples = (int) (S.sim.time_window_size / S.sim.DT);
    int iters_since_last_flush = 1;

    /* Here we go */
    while (S.sim.time < S.sim.total_time) {
        simulate_one_step(&S);
        if (iters_since_last_flush == n_skipped_samples) {
            flush_population_rate(&S);
            iters_since_last_flush = 0;
        }
        iters_since_last_flush++;
    }
    save_spike_activity(&S);
    save_pdfs_synaptic_vars(&S.ntw);
    report("\n");
    compute_average_autocorrelations(&S);
    compute_global_autocorrelations(&S);
    free_state(&S);
    return status;
}
