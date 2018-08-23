#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <namaster.h>
#include <omp.h>
#include "utils.h"
#include "timing_utils.h"

double relbeg,relend;

void timer(int i)
{
  if(i==0)
    relbeg=omp_get_wtime();
  else {
    relend=omp_get_wtime();
  }    
}

//Initializes a mask that is all zeros below the equator and one above
double *init_mask(int nside)
{
  long jj,npix=he_nside2npix(nside);
  double *mask=my_malloc(npix*sizeof(double));
  double v[3];
  for(jj=0;jj<npix;jj++) {
    he_pix2vec_ring(nside,jj,v);
    mask[jj]=1;
    if(v[2]>0)
      mask[jj]=1.;
    else
      mask[jj]=0.;
  }

  return mask;
}

//Initializes a set of maps by populating them with random numbers
double **init_maps(int nside,int nmaps) 
{
  int ii;
  long jj,npix=he_nside2npix(nside);
  double **maps=my_malloc(nmaps*sizeof(double *));
  gsl_rng *rng=init_rng(1234);
  for(ii=0;ii<nmaps;ii++) {
    maps[ii]=my_malloc(npix*sizeof(double));
    for(jj=0;jj<npix;jj++)
      maps[ii][jj]=rng_01(rng);
  }
  end_rng(rng);

  return maps;
}

//timing_st constructor
timing_st *timing_st_init(char *name,int nside,int is_1point,
			  void *(*setup_time)(int,int,int),
			  void (*func_time)(void *),
			  void (*free_time)(void *))
{
  timing_st *tim=my_malloc(sizeof(timing_st));
  sprintf(tim->name,"%s",name);
  tim->nside=nside;
  tim->is_1point=is_1point;
  tim->setup=setup_time;
  tim->func=func_time;
  tim->free=free_time;
  tim->times[0]=-1;
  tim->times[1]=-1;
  tim->times[2]=-1;
  return tim;
}

//timing_st destructor
void timing_st_free(timing_st *tim)
{
  free(tim);
}

//timing_st timer
void timing_st_time(timing_st *tim,int ncomp)
{
  int pol2max=1;
  if(tim->is_1point)
    pol2max=0;

  int pol2;
  int ind_write=0;
  for(pol2=0;pol2<=pol2max;pol2++) {
    int pol1;
    for(pol1=pol2;pol1<=1;pol1++) {
      int ii;
      void *data=tim->setup(tim->nside,2*pol1,2*pol2);
      timer(0);
      for(ii=0;ii<ncomp;ii++) {
	tim->func(data);
      }
      timer(1);
      tim->free(data);
      tim->times[ind_write]=1000.*(relend-relbeg)/ncomp;
      ind_write++;
    }
  }
}

//Report timings
void timing_st_report(timing_st *tim,FILE *f)
{
  int nnodes=omp_get_num_threads();
  fprintf(f,"%s %d %d %.3lE %.3lE %.3lE\n",tim->name,nnodes,tim->nside,
	  tim->times[0],tim->times[1],tim->times[2]);
}

//Times a given function
double timing(int ncomp,int nside,int spin1,int spin2,
			  void *(*setup_time)(int,int,int),
			  void (*func_time)(void *),
			  void (*free_time)(void *))
{
  int ii;
  void *data=setup_time(nside,spin1,spin2);

  timer(0);
  for(ii=0;ii<ncomp;ii++) {
    func_time(data);
  }
  timer(1);

  free_time(data);

  return 1000.*(relend-relbeg)/ncomp;
}
