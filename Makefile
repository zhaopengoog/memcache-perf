ifndef LIBPATH
LIBPATH=$(shell echo "$$HOME/clouds/lib")
endif
LIBPATHFLAG=-L$(LIBPATH)
ifndef INCPATH
INCPATH=$(shell echo "$$HOME/clouds/include")
endif
INCPATHFLAG=-I$(INCPATH)
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

mutilate: $(OBJS)
	export LD_RUN_PATH=$(LIBPATH) && g++ -o mutilate $(OBJS) $(LIBPATHFLAG) $(LIBS)

clean:
	rm -f *.o mutilate

release: mutilate.tgz
	cp mutilate.tgz ../ansible/roles/memcache_clients/files

mutilate.tgz: $(SRCS)
	tar --transform=s,^,mutilate/, -cvzf mutilate.tgz *.cc *.h Makefile COPYING README.md 
