#include "GPS/Metrics/gps_result_sort.h"

// Legacy-compatible sort helpers.
// The metric arrays keep related metadata in parallel arrays, so these routines
// swap every associated field whenever a speed/result value moves.

void sort_display(double a[],int size){
  for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                    }
        }
  }     
}


void sort_run(double a[], uint8_t hour[], uint8_t minute[],uint8_t seconde[],uint8_t mean_cno[],uint8_t max_cno[],uint8_t min_cno[],uint8_t nrSats[],int runs[], int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];int b=hour[o];int c=minute[o];int d=seconde[o];int e=runs[o];int f=mean_cno[o];int g=max_cno[o];int h=min_cno[o];int j=nrSats[o];
                    a[o] = a[o+1];hour[o] = hour[o+1];minute[o] = minute[o+1];seconde[o]=seconde[o+1];runs[o]=runs[o+1];mean_cno[o]=mean_cno[o+1];max_cno[o]=max_cno[o+1];min_cno[o]=min_cno[o+1];nrSats[o]=nrSats[o+1];
                    a[o+1] = t; hour[o+1] = b; minute[o+1] = c;seconde[o+1]=d;runs[o+1]=e;mean_cno[o+1]=f;max_cno[o+1]=g;min_cno[o+1]=h;nrSats[o+1]=j;
                }
        }
    }
}


void sort_run_results(double a[], int dis[],int message[],uint8_t hour[], uint8_t minute[],uint8_t seconde[],int runs[], int samples[],int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    double t = a[o];int v=dis[o];int x=message[o];int b=hour[o];int c=minute[o];int d=seconde[o];int e=runs[o];int f=samples[o];
                    a[o] = a[o+1];dis[o] = dis[o+1];message[o]=message[o+1];hour[o] = hour[o+1];minute[o] = minute[o+1];seconde[o]=seconde[o+1];runs[o]=runs[o+1];samples[o]=samples[o+1];
                    a[o+1] = t; dis[o+1] = v;message[o+1]=x;hour[o+1] = b; minute[o+1] = c;seconde[o+1]=d;runs[o+1]=e;samples[o+1]=f;
                }
        }
    }
}
