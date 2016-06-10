/* parser.c
 * Dani Martí, Aug 2013
 *             Mar 2014 added option and config parsing
 *
 * Adapted from the parser by Stefano Fusi */
#include "parser.h"

/* Auxiliary functions */

bool is_numeric (const char * s)
{
        if (s == NULL || *s == '\0')
                return false;
        char * p;
        strtod (s, &p);
        return *p == '\0';
}

/* Strip whitespace chars off end of given string, in place. Return s.  */
char *rstrip(char *s)
{
        char *p = s + strlen(s);
        while (p > s && isspace(*--p))
                *p = '\0';
        return s;
}

/* Return pointer to first non-whitespace char in given string.  */
char *lskip(char *s)
{
        while (*s && isspace(*s))
                s++;
        return (char *) s;
}

/* Return pointer to the unquoted string */
char *unquote(char *s)
{
        char *p = s + strlen(s);
        if (*--p == '"' && *s == '"') {
                s++;
                *p = '\0';
        }
        return s;
}

/* Return pointer to first char c or '#' in given string, or pointer to 
 * null at end of string if neither found.  */
char *find_char_or_comment(char *s, char c)
{
        while (*s && *s != c && *s != '#')
                s++;
        return (char *) s;
}

/* Version of strncpy that ensures dest (size bytes) is null-terminated. */
char *strncpy0(char *dest, const char *src, size_t size)
{
        strncpy(dest, src, size);
        dest[size - 1] = '\0';
        return dest;
}

char *get_contents_in_brackets(char *s, int n)
{
        char *r;
        char *t;
        s += n;                     /* Skip field name */
        s = lskip(s);               /* Skip white chars */
        if (*s == '[') {            /* See what's between square brackets */
                s++;
                s = lskip(s);
                r = s;
                while (*r && *r != ']')
                        r++;
                t = r;
                t++;
                /* Check that there are open braces */
                if (find_char_or_comment(t, '{')) {
                        *r = '\0';
                        s = rstrip(s);
                        return s;
                } else {
                        report("Error: no opening braces.\n");
                        return NULL;
                }
        } else {
                printf("Error: no square brackets.\n");
                return NULL;
        }
}

int parse_config_file(const char *filename, struct State *S)
{
        FILE *fin;
        char buf[MAX_LINE];

        char *s;
        char *r;
        char *name;
        char *value;
        double tmp;

        int error = 0;

        fin = fopen(filename, "r");
        if (!fin)
                return -1;

        int lineno = 0;
        struct Network *ntw = &S->ntw;

        /* Scan through file line by line */
        while (fgets(buf, sizeof(buf), fin) != NULL) {
                lineno++;
                s = lskip(rstrip(buf));     /* chop whitespaces off at both sides */
                if (*s && *s != '#') {
                        /* We expect a name = value pair here */
                        r = find_char_or_comment(s, '=');
                        /* First, split the assignment between names and values */
                        if (*r == '=') {
                                *r = '\0';
                                name = rstrip(s);
                                value = lskip(r + 1);
                                r = find_char_or_comment(value, '#');
                                if (*r == '#')
                                        *r = '\0';
                                value = rstrip(value);
                                value = unquote(value);
                                if (strncmp(name, "N", 1) == 0) {
                                        ntw->N = atoi(value);
                                } else if (strncmp(name, "f", 1) == 0) {
                                        tmp = atof(value);
                                        if (tmp < 0 || tmp > 1)
                                                report("Parameter f must be in the interval [0,1]\n");
                                        ntw->NE = (int) ntw->N * atof(value);
                                        ntw->NI = ntw->N - ntw->NE;
                                } else if (strncmp(name, "tau_m", 5) == 0) {
                                        ntw->tau_m = atof(value);
                                } else if (strncmp(name, "tau_rp", 6) == 0) {
                                        ntw->tau_rp = atof(value);
                                } else if (strncmp(name, "J", 1) == 0) {
                                        ntw->J = atof(value);
                                } else if (strncmp(name, "g", 1) == 0) {
                                        ntw->g = atof(value);
                                } else if (strncmp(name, "C", 1) == 0) {
                                        ntw->C = atoi(value);
                                } else if (strncmp(name, "tau_fast", 5) == 0) {
                                        ntw->tau_fast = atof(value);
                                } else if (strncmp(name, "tau_slow", 5) == 0) {
                                        ntw->tau_slow = atof(value);
                                        ntw->slow_flag = true;
                                } else if (strncmp(name, "delay", 5) == 0) {
                                        ntw->delay = atof(value);
                                } else if (strncmp(name, "ext_current", 11) == 0) {
                                        ntw->ext_current = atof(value);
                                } else {
                                        report ("Line %d: Invalid value pair in this context\n", lineno);
                                        return lineno;
                                }
                        } /* if (*r == '=') */
                } /* Not a comment or blank line */
        } /* EOF */
        fclose(fin);
        return error;
}

void usage(int status, char *s)
{
        if (status != 0) {
                fprintf(stderr, "Usage: %s [OPTIONS]\n", s);
                fprintf(stderr, "Try `%s --help' for more information.\n", s);
        } else {
                printf("Usage: %s [OPTIONS]\n\n", s);
                printf("Simulate a network of spiking neurons and save the resulting spikes,\n\
the population rates, and the autocorrelations.\n\n\
  General settings:\n\
    -c, --config-file=FILE              read configuration parameters from FILE\n\
    -v, --verbose                       be verbose.\n\n\
  Network parameters:\n\
    -N, --number-of-neurons=INT         set the total number of neurons\n\
    -C, --number-of-connections=INT     set the number of connections per cell\n\
    -f, --fraction-excitatory=REAL      set the fraction of excitatory cells in the whole network\n\
    -J, --synaptic-efficacy=REAL        set the synaptic efficacy of excitatory connections\n\
    -g, --inh-to-exc-weight-ratio=REAL  set the ratio between inhibitory to excitatory efficacies (absolute value)\n\
    -T, --membrane-time-constant=REAL   set the membrane time constant (in ms)\n\
    -r, --refractory-period=REAL        set the refractory period (in ms) \n\
    -D, --synaptic-delay=REAL           set the synaptic delay (in ms) \n\
    -t, --synaptic-time-constant=REAL   set the time constant of fast synapses\n\
    -I, --external-current=REAL         set the homogeneous external current (in mV)\n\n\
  Miscellaneous:\n\
    -h, --help                          display this help and exit\n\n");
                exit(status);
        }
}

static struct option long_opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {"config-file", required_argument, NULL, 'c'},
        {"number-of-neurons", required_argument, NULL, 'N'},
        {"number-of-connections", required_argument, NULL, 'C'},
        {"fraction-excitatory", required_argument, NULL, 'f'},
        {"synaptic-efficacy", required_argument, NULL, 'J'},
        {"inh-to-exc-weight-ratio ", required_argument, NULL, 'g'},
        {"membrane-time-constant", required_argument, NULL, 'T'},
        {"refractory-period", required_argument, NULL, 'r'},
        {"synaptic-delay", required_argument, NULL, 'D'},
        {"synaptic-time-constant", required_argument, NULL, 't'},
        {"ext-current", required_argument, NULL, 'I'},
        {0, 0, 0, 0}
};


int process_options(void *s, int argc, char *argv[])
{
        struct State *S = (struct State *) s;

        struct Network *ntw = &S->ntw;
        struct Simulation *sim = &S->sim;

        int c;

        int option_index = 0;       /* getopt_long stores the option index here. */
        double tmp;

        opterr = 1;
        optind = 1; /* reset the counter (extern) */

        while ((c = getopt_long(argc, argv, "hvc:N:C:f:J:g:T:r:D:t:I:",
                                        long_opts, &option_index)) != -1) {

                switch (c) {
                        case 0:
                                if (strcmp(long_opts[option_index].name, "help") == 0) {
                                        usage(0, argv[0]);
                                        return 0;
                                } else if (strcmp(long_opts[option_index].name, "config-file") == 0) {
                                        strcpy(sim->config_file, optarg);
                                        if (parse_config_file(sim->config_file, S) < 0)
                                                printf("'%s' cannot be loaded. Using default values and options.",
                                                        sim->config_file);
                                } else if (strcmp(long_opts[option_index].name, "verbose") == 0) {
                                        sim->verbose = true;
                                } else if (strcmp(long_opts[option_index].name, "number-of-neurons") == 0) {
                                        ntw->N = atoi(optarg);
                                } else if (strcmp(long_opts[option_index].name, "number-of-connections") == 0) {
                                        ntw->C = atoi(optarg);
                                } else if (strcmp(long_opts[option_index].name, "membrane-time-constant") == 0) {
                                        ntw->tau_m = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "refractory-period") == 0) {
                                        ntw->tau_rp = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "synaptic-efficacy") == 0) {
                                        ntw->J = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "fraction-excitatory") == 0) {
                                        tmp = atof(optarg);
                                        if (tmp < 0 || tmp > 1)
                                                report("Parameter f must be in the interval [0,1]\n");
                                        ntw->NE = (int) ntw->N * atof(optarg);
                                        ntw->NI = ntw->N - ntw->NE;
                                } else if (strcmp(long_opts[option_index].name, "inh-to-exc-weight-ratio") == 0) {
                                        ntw->g = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "synaptic-delay") == 0) {
                                        ntw->delay = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "synaptic-time-constant") == 0) {
                                        ntw->tau_fast = atof(optarg);
                                } else if (strcmp(long_opts[option_index].name, "external-current") == 0) {
                                        ntw->ext_current = atof(optarg);
                                }
                                break;
                        case 'h':
                                usage(0, argv[0]);
                                return 0;
                        case 'v':
                                sim->verbose = true;
                                break;  /* we took care of that in process_config_in_options */
                        case 'c':
                                strcpy(sim->config_file, optarg); /* Parse the configuration file */
                                if (parse_config_file(sim->config_file, S) < 0)
                                        printf("'%s' cannot be loaded. Using default values and options.",
                                                        sim->config_file);
                                break;  /* we took care of that in process_config_in_options */
                        case 'N':
                                ntw->N = atoi(optarg);
                                break;
                        case 'C':
                                ntw->C = atoi(optarg);
                                break;
                        case 'T':
                                ntw->tau_m = atof(optarg);
                                break;
                        case 'r':
                                ntw->tau_rp = atof(optarg);
                                break;
                        case 'J':
                                ntw->J = atof(optarg);
                                break;
                        case 'f':
                                tmp = atof(optarg);
                                if (tmp < 0 || tmp > 1)
                                        report("Parameter f must be in the interval [0,1]\n");
                                ntw->NE = (int) ntw->N * atof(optarg);
                                ntw->NI = ntw->N - ntw->NE;
                                break;
                        case 'g':
                                ntw->g = atof(optarg);
                                break;
                        case 't':
                                ntw->tau_fast = atof(optarg);
                                break;
                        case 'D':
                                ntw->delay = atof(optarg);
                                break;
                        case 'I':
                                ntw->ext_current = atof(optarg);
                                break;
                        default:
                                printf("Invalid option -- '%s'\n", optarg);
                                printf("Try <exec> --help for more information\n");
                                abort();
                }
        }
        if (argc - option_index != 2) {
                return 1;
        }
        return 0;
}

int read_network_parameters(int argc, char *argv[], struct State *S)
{
        int status = 0;

        /* Read the defaults */
        if (parse_config_file(S->sim.config_file, S) < 0)
                printf("'%s' cannot be loaded. Using default values and options.",
                                S->sim.config_file);

        status = process_options(S, argc, argv);

        if (S->sim.verbose) {
                show_parameters(S); /* Do something */
        }
        return status;
}
