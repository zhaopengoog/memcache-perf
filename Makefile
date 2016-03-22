VERSION=0.1
LIBS=-levent -lpthread -lrt -lzmq
CXXFLAGS= -g -std=c++0x -D_GNU_SOURCE -O3 $(INCPATHFLAG)
HEADERS= AdaptiveSampler.h barrier.h cmdline.h Connection.h ConnectionStats.h \
 Generator.h log.h mutilate.h util.h AgentStats.h binary_protocol.h \
 config.h ConnectionOptions.h distributions.h \
 HistogramSampler.h LogHistogramSampler.h Operation.h cpu_stat_thread.h
CFILES= barrier.cc  cmdline.cc  Connection.cc  distributions.cc  \
 Generator.cc  log.cc  mutilate.cc  TestGenerator.cc  util.cc cpu_stat_thread.cc
SRCS=$(HEADERS) $(CFILES)
OBJS=mutilate.o cmdline.o log.o distributions.o util.o Connection.o Generator.o cpu_stat_thread.o


mcperf: $(OBJS)
	export LD_RUN_PATH=$(LIBPATH) && g++ -o mcperf $(OBJS) $(LIBPATHFLAG) $(LIBS)

clean:
	rm -f *.o mutilate

memcache-perf.tgz: $(SRCS)
	tar --transform=s,^,memcache-perf-$(VERSION)/, -cvzf memcache-perf-$(VERSION).tgz *.cc *.h Makefile COPYING README.md 

%.d: %.cc
	@set -e; rm -f $@; \
	$(CC) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(CFILES:.cc=.d)
