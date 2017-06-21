// -*- c++ -*-

// 1. implement "fixed" generator
// 2. implement discrete generator
// 3. implement combine generator? 

#ifndef KEYGENERATOR_H
#define KEYGENERATOR_H

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include "config.h"
#include "Generator.h"
#include <string>
#include <vector>
#include <utility>

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "util.h"

#define max_memcached_len 250

class KeyGenerator {
public:
  KeyGenerator(Generator* _g, double _max = 10000) : g(_g), max(_max) {
	mlen=floor(log10(max)) + 1;
  }
  int keysize(uint64_t h) {
    double U = (double) h / ULLONG_MAX;
    double G = g->generate(U);
    int keylen = MAX(round(G), mlen);
	return keylen;
  }
  virtual std::string generate(uint64_t ind) {
    uint64_t h = fnv_64(ind);
    int keylen = keysize(h);
    char key[max_memcached_len];
    snprintf(key, max_memcached_len, "%0*" PRIu64, keylen, ind);
    return std::string(key);
  }
protected:
  Generator* g;
  double max;
  int mlen;
};

class DistKeyGenerator : public KeyGenerator {
	//Instead of generating a specific index, each gen will get an index according to distribution provided
	//The generator(kg) is assumed to be zipfian or pareto. This should not be used when seeding the data,
	//instead meant to enable the request order of indices to be zipfian.
	//To that end, index parameter is ignored, and a generated index based on the distribution is used instead. 
	//
	//params:
	//	kg: distribution for values of the keys (order)
	//	ks: distribtuion for key sizes
	//	max: max number of keys
public:
  DistKeyGenerator(Generator* _ks, Generator* _kg, double _max = 10000) : KeyGenerator(_ks,_max), kg(_kg) {  }
  std::string generate(uint64_t ind) {
	double ridx = drand48();
    ind = (uint64_t)kg->generate(ridx) % max;
    uint64_t h = fnv_64(ind);
    int keylen = keysize(h);
    snprintf(key, max_memcached_len, "%0*" PRIu64, keylen, ind);
    return std::string(key);
  }
private:
  Generator* kg;
  char key[max_memcached_len];
};

/*
	Class: CachingKeyGenerator
	A key generator that creates a cached pool of keys/requests

	Key generation can become a bottleneck if doing lots of small requests,
	requiring large number of clients to saturate a server.
	This class will build up a cache of requests and repeat sending those requests. Each client thread is going to have a different req seq, and the generator will regenerate some of the sequence every max_iterations over the data.

	_capacity: size of the cache pool (default 10k)
	max_iterations: how many times can the pool be reused before regenerating, defaults to 100. Set to 1 to always invoke regen.
	regen_freedom: controls how much of the pool will be regenerated on each regen. Set to capacity to make each regen update the whole cache. Lower values will update fewer requests each reen cycle. Defaults to 1% of capacity.
*/
class CachingKeyGenerator {
private:
	std::vector< std::string > values;
	std::vector< std::string > get_req;
	uint64_t capacity,max;
	KeyGenerator *kg;
	void commonInit(int reuse, int pct_regen) {
		if (capacity>max)
			capacity=max;
		values.resize(capacity);
		get_req.resize(capacity);
		step=1;
		iterations=0;
		max_iterations=reuse;
		regen_freedom=capacity*pct_regen/100;
	}
	
public:
	int next,step;
	int iterations;
	int max_iterations;
	int regen_freedom;

public:
	CachingKeyGenerator(Generator* _ks, Generator* _kg, uint64_t _max=40000, int _capacity=10000, int reuse=100, int pct_regen=1) : capacity(_capacity), max(_max) {
		commonInit(reuse,pct_regen);
		if (_kg != NULL)
			kg=new DistKeyGenerator(_ks, _kg, max);
		else
			kg=new KeyGenerator(_ks, max);
		regen();
	}
	~CachingKeyGenerator() {
		delete kg;
	}
	std::string generate(uint64_t ind) {
		return values[ind];
	}
	const char *current_get_req() {
		return get_req[next].c_str();
	}
	const char *generate_next() {
		next+=step;
		if (next >= capacity) {
			next=0;
			iterations++;
			if (iterations > max_iterations) {
				unsigned int regen_offset = rand() % (capacity / regen_freedom);
				unsigned int regen_step = rand() % (capacity / regen_freedom) + 1;
				regen(regen_step,regen_offset);
				iterations=0;
			}
		}
		return values[next].c_str();
	}
	void regen(unsigned int stepper=1, unsigned int offset=0) {
		for (uint64_t i=offset ; i<capacity; i+=stepper) {
			values[i] = kg->generate(i);
			get_req[i] = std::string("get ") + values[i] + std::string("\r\n"); 
		}
		next=0;
	}
};

#endif // KEYGENERATOR_H
