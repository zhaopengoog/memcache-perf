// -*- c++ -*-

// 1. implement "fixed" generator
// 2. implement discrete generator
// 3. implement combine generator? 

#ifndef GENERATOR_H
#define GENERATOR_H

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#include "config.h"

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

// Generator syntax:
//
// \d+ == fixed
// n[ormal]:mean,sd
// e[xponential]:lambda
// p[areto]:scale,shape
// g[ev]:loc,scale,shape
// fb_value, fb_key, fb_rate

class Generator {
public:
  Generator() {}
  //  Generator(const Generator &g) = delete;
  //  virtual Generator& operator=(const Generator &g) = delete;
  virtual ~Generator() {}

  virtual double generate(double U = -1.0) = 0;
  virtual void set_lambda(double lambda) {DIE("set_lambda() not implemented");}
protected:
  std::string type;
};

class Fixed : public Generator {
public:
  Fixed(double _value = 1.0) : value(_value) { D("Fixed(%f)", value); }
  virtual double generate(double U = -1.0) { return value; }
  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) value = 1.0 / lambda;
    else value = 0.0;
  }

private:
  double value;
};

class Uniform : public Generator {
private:
  double min;
  double max;
  double scale;

public:
  Uniform(double _scale) { 
	scale=_scale; 
	min=0; 
	max=scale; 
	D("Uniform(%f)", scale); 
  }
  Uniform(double _min, double _max) { 
	if (_max<=0.0) { /* in case max is undefined, go 0 .. defined val */
		max=_min;
		min=0.0;
 	} else {
		min=_min; 
		max=_max;
	}
	scale=max-min; 
	D("Uniform(%f..%f)", min,max); 
  }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    return scale * U + min;
  }

  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) scale = 2.0 / lambda;
    else scale = 0.0;
  }

};

class Normal : public Generator {
public:
  Normal(double _mean = 1.0, double _sd = 1.0) : mean(_mean), sd(_sd) {
    D("Normal(mean=%f, sd=%f)", mean, sd);
  }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    double V = U; // drand48();
    double N = sqrt(-2 * log(U)) * cos(2 * M_PI * V);
    return mean + sd * N;
  }

  virtual void set_lambda(double lambda) {
    if (lambda > 0.0) mean = 1.0 / lambda;
    else mean = 0.0;
  }

private:
  double mean, sd;
};

class Exponential : public Generator {
public:
  Exponential(double _lambda = 1.0) : lambda(_lambda) {
    D("Exponential(lambda=%f)", lambda);
  }

  virtual double generate(double U = -1.0) {
    if (lambda <= 0.0) return 0.0;
    if (U < 0.0) U = drand48();
    return -log(U) / lambda;
  }

  virtual void set_lambda(double lambda) { this->lambda = lambda; }

private:
  double lambda;
};

class GPareto : public Generator {
public:
  GPareto(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
    loc(_loc), scale(_scale), shape(_shape) {
    assert(shape != 0.0);
    D("GPareto(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
  }

  virtual double generate(double U = -1.0) {
    if (U < 0.0) U = drand48();
    return loc + scale * (pow(U, -shape) - 1) / shape;
  }

  virtual void set_lambda(double lambda) {
    if (lambda <= 0.0) scale = 0.0;
    else scale = (1 - shape) / lambda - (1 - shape) * loc;
  }

private:
  double loc /* mu */;
  double scale /* sigma */, shape /* k */;
};

class GEV : public Generator {
public:
  GEV(double _loc = 0.0, double _scale = 1.0, double _shape = 1.0) :
    e(1.0), loc(_loc), scale(_scale), shape(_shape) {
    assert(shape != 0.0);
    D("GEV(loc=%f, scale=%f, shape=%f)", loc, scale, shape);
  }

  virtual double generate(double U = -1.0) {
    return loc + scale * (pow(e.generate(U), -shape) - 1) / shape;
  }

private:
  Exponential e;
  double loc /* mu */, scale /* sigma */, shape /* k */;
};

class Discrete : public Generator {
public:
  ~Discrete() { delete def; }
  Discrete(Generator* _def = NULL) : def(_def) {
    if (def == NULL) def = new Fixed(0.0);
  }

  virtual double generate(double U = -1.0) {
    double Uc = U;
    if (pv.size() > 0 && U < 0.0) U = drand48();

    double sum = 0;
	std::vector< std::pair<double,double> >::iterator p; 
    for (p= pv.begin(); p!= pv.end(); p++ )  {
      sum += (*p).first;
      if (U < sum) return (*p).second;
    }

    return def->generate(Uc);
  }

  void add(double p, double v) {
    pv.push_back(std::pair<double,double>(p, v));
  }

private:
  Generator *def;
  std::vector< std::pair<double,double> > pv;
};

class KeyGenerator {
public:
  KeyGenerator(Generator* _g, double _max = 10000) : g(_g), max(_max) {}
  std::string generate(uint64_t ind) {
    uint64_t h = fnv_64(ind);
    double U = (double) h / ULLONG_MAX;
    double G = g->generate(U);
    int keylen = MAX(round(G), floor(log10(max)) + 1);
    char key[256];
    snprintf(key, 256, "%0*" PRIu64, keylen, ind);

    //    D("%d = %s", ind, key);
    return std::string(key);
  }
private:
  Generator* g;
  double max;
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
	uint64_t capacity;
	KeyGenerator *kg;
	
public:
	int next,step;
	int iterations;
	int max_iterations;
	int regen_freedom;

public:
	CachingKeyGenerator(Generator* _g, uint64_t max=4096, int _capacity=10000, int reuse=100, int pct_regen=1) : capacity(_capacity) {
		values.resize(capacity);
		get_req.resize(capacity);
		kg=new KeyGenerator(_g, max);
		step=1;
		iterations=0;
		max_iterations=reuse;
		regen_freedom=capacity*pct_regen/100;
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

Generator* createGenerator(std::string str);
Generator* createFacebookKey();
Generator* createFacebookValue();
Generator* createFacebookIA();

#endif // GENERATOR_H
