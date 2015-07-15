import os
srcs = Split("""parameters.c
                simulation.c
                network.c
                parser.c""")

# segfault = '-v -da -Q'
cflags = '-std=gnu99 -O3 --pedantic -W -Wall -Winline -Werror \
-Wstrict-prototypes -Wno-sign-conversion -Wshadow -Wpointer-arith -Wcast-qual \
-Wcast-align -Wwrite-strings -Wnested-externs -Wno-unused-result \
-fshort-enums -fno-common'

src_path = os.path.join(os.environ['HOME'], 'src')
lpath = ['.', os.path.join(src_path, 'lib')]
opt = Environment(CFLAGS = cflags + ' -DHAVE_INLINE=1',
                  CPPPATH=os.path.join(src_path, 'include'))
libs = ['network', 'eprintf', 'm', 'gsl', 'gslcblas']
opt.Library('network', srcs)
opt.Program('simulate_one_trial.c', LIBS=libs, LIBPATH=lpath)
