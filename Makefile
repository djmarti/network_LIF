SOURCES = network.c parameters.c parser.c simulation.c
OBJS = $(SOURCES:.c=.o)
CFLAGS = -std=gnu99 -O3 -DHAVE_INLINE=1 --pedantic -W -Wall -Winline -Werror \
	 -Wstrict-prototypes -Wno-sign-conversion -Wshadow -Wpointer-arith -Wcast-qual \
	 -Wcast-align -Wwrite-strings -Wnested-externs -Wno-unused-result \
	 -fshort-enums -fno-common
LIBS = -lm -lgsl -lgslcblas -lnetwork -leprintf

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

MAIN = simulate_one_trial

all: libnetwork.a libeprintf.a $(MAIN) 

libeprintf.a: eprintf.o
	$(AR) rcs $@ $^

libnetwork.a: $(OBJS)
	$(AR) rcs $@ $^

$(MAIN): simulate_one_trial.o
	$(CC) -o $@ $^ -L. $(LIBS)

clean:
	rm -f simulate_one_trial.o $(OBJS) libeprintf.a libnetwork.a
