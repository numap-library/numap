#include "numap.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

pthread_barrier_t barrier;
pid_t tid_0, tid_1;

void to_be_profiled() {
  long size = 50000000;
  int *data = malloc(sizeof(int) * size);
  if (data == NULL) {
    printf("malloc failed\n");
    exit(-1);
  }
  int res = 0;
  for (long i = 0; i < size; i++) {
    data[i] = i;
    res += data[i];
  }
}

void *thread_0_f(void *p) {
  tid_0 = syscall(SYS_gettid);
  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier);
  to_be_profiled();
  pthread_barrier_wait(&barrier);
  return NULL;
}

void *thread_1_f(void *p) {
  tid_1 = syscall(SYS_gettid);
  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier);
  to_be_profiled();
  pthread_barrier_wait(&barrier);
  return NULL;
}

#define T0_CPU 2
#define T1_CPU 3

int main() {

  // Init numap
  int res = numap_init();
  if(res < 0) {
    fprintf(stderr, "numap_init : %s\n", numap_error_message(res));
    return -1;
  }

  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if (T0_CPU >= num_cores || T1_CPU >= num_cores) {
    fprintf(stderr, "Can't set affinity to thread 0 to %d or to thread 1 to %d\n", T0_CPU, T1_CPU);
    return -1;
  }

  // Create threads
  res = pthread_barrier_init(&barrier, NULL, 3);
  if (res) {
    fprintf(stderr, "Error creating barrier: %d\n", res);
    return -1;
  }
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(T0_CPU, &mask);
  res = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
  if (res != 0) {
    fprintf(stderr, "Error setting affinity to thread 0: %s\n", strerror(res));
    return -1;
  }
  pthread_t thread_0;
  if ((res = pthread_create(&thread_0, &attr, thread_0_f, (void *)NULL)) < 0) {
    fprintf(stderr, "Error creating thread 0: %d\n", res);
    return -1;
  }
  CPU_ZERO(&mask);
  CPU_SET(T1_CPU, &mask);
  res = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
  if (res != 0) {
    fprintf(stderr, "Error setting affinity to thread 1: %s\n", strerror(res));
    return -1;
  }
  pthread_t thread_1;
  if ((res = pthread_create(&thread_1, &attr, thread_1_f, (void *)NULL)) < 0) {
    fprintf(stderr, "Error creating thread 1: %d\n", res);
    return -1;
  }

  // Init read sampling
  struct numap_sampling_measure sm;
  int sampling_rate = 1000;
  res = numap_sampling_init_measure(&sm, 2, sampling_rate, 64);
  if(res < 0) {
    fprintf(stderr, "numap_sampling_init error : %s\n", numap_error_message(res));
    return -1;
  }
  pthread_barrier_wait(&barrier);
  sm.tids[0] = tid_0;
  sm.tids[1] = tid_1;

  // Start memory read access sampling
  printf("\nStarting memory read sampling");
  fflush(stdout);
  // has to be called after tids set and before start
  res = numap_sampling_read_start(&sm);
  if(res < 0) {
    fprintf(stderr, " -> numap_sampling_start error : %s\n", numap_error_message(res));
    return -1;
  }
  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier);

  // Stop memory read access sampling
  res = numap_sampling_read_stop(&sm);
  if(res < 0) {
    printf("numap_sampling_stop error : %s\n", numap_error_message(res));
    return -1;
  }

  // Print memory read sampling results
  printf("\nMemory read sampling results\n");
  numap_sampling_read_print(&sm, 0);

  if ((res = pthread_create(&thread_0, &attr, thread_0_f, (void *)NULL)) < 0) {
    fprintf(stderr, "Error creating thread 0: %d\n", res);
    return -1;
  }
  CPU_ZERO(&mask);
  CPU_SET(T1_CPU, &mask);
  res = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
  if (res != 0) {
    fprintf(stderr, "Error setting affinity to thread 1: %s\n", strerror(res));
    return -1;
  }
  if ((res = pthread_create(&thread_1, &attr, thread_1_f, (void *)NULL)) < 0) {
    fprintf(stderr, "Error creating thread 1: %d\n", res);
    return -1;
  }

  // Init write sampling
  res = numap_sampling_init_measure(&sm, 2, sampling_rate, 64);
  if(res < 0) {
    fprintf(stderr, "numap_sampling_init error : %s\n", numap_error_message(res));
    return -1;
  }
  pthread_barrier_wait(&barrier);
  sm.tids[0] = tid_0;
  sm.tids[1] = tid_1;

  // Start memory write access sampling
  printf("\nStarting memory write sampling");
  fflush(stdout);
  res = numap_sampling_write_start(&sm);
  if(res < 0) {
    fprintf(stderr, " -> numap_sampling_start error : %s\n", numap_error_message(res));
    return -1;
  }
  pthread_barrier_wait(&barrier);
  pthread_barrier_wait(&barrier);

  // Stop memory write access sampling
  res = numap_sampling_write_stop(&sm);
  if(res < 0) {
    printf("numap_sampling_stop error : %s\n", numap_error_message(res));
    return -1;
  }

  // Print memory write sampling results
  printf("\nMemory write sampling results\n");
  numap_sampling_write_print(&sm, 0);

  /* // Start memory controler read and writes counting */
  /* struct numap_bdw_measure m; */
  /* res = numap_bdw_init_measure(&m); */
  /* if(res < 0) { */
  /*   fprintf(stderr, "numap_bdw_init error : %s\n", numap_error_message(res)); */
  /*   return -1; */
  /* } */
  /* res = numap_bdw_start(&m); */
  /* if(res < 0) { */
  /*   fprintf(stderr, "numap_bdw_start error : %s\n", numap_error_message(res)); */
  /*   return -1; */
  /* } */

  /* // Stop memory controler read and writes counting */
  /* res = numap_stop(&m); */
  /* if(res < 0) { */
  /*   printf("numap_stop error : %s\n", numap_error_message(res)); */
  /*   return -1; */
  /* } */

  /* // Print memory counting results */
  /* printf("Memory counting results\n"); */
  /* for(int i = 0; i < m.nb_nodes; i++) { */
  /*   printf("Memory reads  count for node %d is %lld\n", i, m.reads_count[i]); */
  /*   printf("Memory writes count for node %d is %lld\n", i, m.writes_count[i]); */
  /* } */

  return 0;
}
