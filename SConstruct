import os
srcs = Split("""parameters.c
                simulation.c
                network.c
                parser.c
                """)

cflags = '-std=gnu99 -O3 --pedantic -W -Wall -Winline -Werror \
-Wstrict-prototypes -Wno-sign-conversion -Wshadow -Wpointer-arith -Wcast-qual \
-Wcast-align -Wwrite-strings -Wnested-externs -Wno-unused-result \
-fshort-enums -fno-common'

opt = Environment(CFLAGS = cflags + ' -DHAVE_INLINE=1')
libs = ['network', 'eprintf', 'm', 'gsl', 'gslcblas']
opt.Library('network', srcs)
opt.Library('eprintf', 'eprintf.c')
opt.Program('simulate_one_trial.c', LIBS=libs, LIBPATH=['.'])
