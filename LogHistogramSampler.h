/* -*- c++ -*- */
#ifndef LOGHISTOGRAMSAMPLER_H
#define LOGHISTOGRAMSAMPLER_H

#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include <vector>

#include "mcperf.h"
#include "Operation.h"

//increase resolution slightly for the range of ~10ms
#define _POW 1.08
//at bin 200 the latency is >4s
#define LOGSAMPLER_BINS 200
#ifdef GNUPLOT
#include "gnuplot_i.h"
static int nm=0;
#endif

class LogHistogramSampler {
public:
  std::vector<uint64_t> bins;

  std::vector<Operation> samples;

  double sum;
  double sum_sq;

  LogHistogramSampler() = delete;
  LogHistogramSampler(int _bins) : sum(0.0), sum_sq(0.0) {
    assert(_bins > 0);

    bins.resize(_bins + 1, 0);
  }

  void sample(const Operation &op) {
    sample(op.time());
    if (args.save_given) samples.push_back(op);
  }

  void sample(double s) {
    assert(s >= 0);
    size_t bin = log(s)/log(_POW);

    sum += s;
    sum_sq += s*s;

    //    I("%f", sum);

    if ((int64_t) bin < 0) {
      bin = 0;
    } else if (bin >= bins.size()) {
      bin = bins.size() - 1;
    }

    bins[bin]++;
  }

  double average() {
    //    I("%f %d", sum, total());
    return sum / total();
  }

  double stddev() {
    //    I("%f %d", sum, total());
    return sqrt(sum_sq / total() - pow(sum / total(), 2.0));
  }

  double minimum() {
    for (size_t i = 0; i < bins.size(); i++)
      if (bins[i] > 0) return pow(_POW, (double) i + 0.5);
    DIE("Not implemented");
  }

  double get_nth(double nth) {
    uint64_t count = total();
    uint64_t n = 0;
    double target = count * nth/100;
    if (nth>100.0) {
	target = count * nth/1000;
    }
    if (nth>1000.0) {
	target = count * nth/10000;
    }

    for (size_t i = 0; i < bins.size(); i++) {
      n += bins[i];

      if (n > target) { // The nth is inside bins[i].
        double left = target - (n - bins[i]);
        return pow(_POW, (double) i) +
          left / bins[i] * (pow(_POW, (double) (i+1)) - pow(_POW, (double) i));
      }
    }

    return pow(_POW, bins.size());
  } 

  uint64_t total() {
    uint64_t sum = 0.0;
	std::vector<uint64_t>::iterator i;

    for (i = bins.begin(); i!=bins.end(); i++) sum += *i;

    return sum;
  }

  void accumulate(const LogHistogramSampler &h) {
    assert(bins.size() == h.bins.size());
    for (size_t i = 0; i < bins.size(); i++) bins[i] += h.bins[i];

    sum += h.sum;
    sum_sq += h.sum_sq;
	std::vector<Operation>::const_iterator hi;

    for (hi=h.samples.begin();  hi!=h.samples.end(); hi++) samples.push_back(*hi);
  }
  void plot(const char *tag, double QPS) {
	if (sum<100) return;
#ifdef GNUPLOT
	gnuplot_ctrl    *   h1;
	char fn[42];
	char plot_name[80];
	int size,i,ifirst,ilast;
	//find start of latency bins
	for (i=0; i<bins.size(); i++) {
		if (bins[i] > 0)
			break;
	}
	ifirst=i;
	for (; i<bins.size(); i++) {
		if ((bins[i] == 0 ) && (bins[i+1] == 0))
			break;
	}
	ilast=i+1;
	V("Plotting bins %d to %d\n",ifirst,ilast);
	//find end of latency bins
	size=ilast-ifirst;

	double *x=(double *)malloc(size * sizeof(double));
	double *y=(double *)malloc(size * sizeof(double));
	//data for the plot
	for (i=ifirst; i<ilast; i++) {
		int id=i-ifirst;
		x[id]=pow(_POW, (double) i) / 1000.0;
		y[id]=bins[i];
	}
	//plot the bins
	h1 = gnuplot_init() ;
	const char *hstyle="impulses";
    	gnuplot_setstyle(h1, (char *)hstyle) ;
	gnuplot_cmd(h1, "set terminal png");
	gnuplot_cmd(h1, "set xtics rotate");
	sprintf(fn,"set output \"histogram_%s_%02d.png\"",tag,nm);
    	gnuplot_cmd(h1, fn);
	sprintf(plot_name,"Latency Histogram (Total=%ldK QPS=%fK)", total() / (1000), QPS/1000.0);
	gnuplot_plot_xy(h1, x, y, size, plot_name) ;
        sprintf(fn,"histogram_%s_%02d.csv",tag,nm);
	gnuplot_write_xy_csv(fn,x,y,size,plot_name);
	nm++;
	gnuplot_close(h1);
	free(x);
	free(y);
#endif
  }

};

#endif // LOGHISTOGRAMSAMPLER_H
