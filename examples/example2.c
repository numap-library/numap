#include "numap.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <linux/version.h>

pthread_barrier_t barrier;
pid_t tid_0, tid_1;

void to_be_profiled() {
  long size = 70000000;
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

struct mem_sampling_backed {
  struct mem_sampling_backed* next;
  int fd;    // corresponding fd
  void* buffer;  // buffer where data is backed up
  size_t buffer_size;
};

pthread_mutex_t msb_lock = PTHREAD_MUTEX_INITIALIZER;
struct mem_sampling_backed *msb;

void handler(struct numap_sampling_measure* measure, int fd)
{
    // search metadata
    int tid_i=-1; // search tid
    for (int i = 0 ; i < measure->nb_threads ; i++)
    {
      if (measure->fd_per_tid[i] == fd)
        tid_i = i;
    }
    if (tid_i == -1)
    {
      fprintf(stderr, "No tid associated with fd %d\n", fd);
      exit(EXIT_FAILURE);
    }
    struct perf_event_mmap_page *metadata_page = measure->metadata_pages_per_tid[tid_i];

    // wrap data_head
    if (metadata_page->data_head > metadata_page->data_size)
    {
      metadata_page->data_head = (metadata_page->data_head % metadata_page->data_size);
    }
    uint64_t head = metadata_page->data_head;
    rmb();
    uint64_t tail = metadata_page->data_tail;
    size_t sample_size;
    if (head > tail) {
      sample_size =  head - tail;
    } else {
      sample_size = (metadata_page->data_size - tail) + head;
    }
    struct mem_sampling_backed *new_msb = malloc(sizeof(struct mem_sampling_backed));
    if (new_msb == NULL) {
      fprintf(stderr, "could not malloc mem_sampling_backed\n");
      exit(EXIT_FAILURE);
    }
    new_msb->fd = fd;
    new_msb->buffer_size = sample_size;
    new_msb->buffer = malloc(new_msb->buffer_size);
    if (new_msb->buffer == NULL) {
      fprintf(stderr, "could not malloc buffer\n");
      exit(EXIT_FAILURE);
    }
    // TODO : Save the data here
    //struct perf_event_header *header = (struct perf_event_header *)((char *)metadata_page + measure->page_size + tail);
    uint8_t* start_addr = (uint8_t *)metadata_page;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
    start_addr += metadata_page->data_offset;
#else
    static size_t page_size = 0;
    if(page_size == 0)
      page_size = (size_t)sysconf(_SC_PAGESIZE);
    start_addr += page_size;
#endif
    //void* start_address = (char*)metadata_page+measure->page_size+tail;
    if (head > tail) {
      memcpy(new_msb->buffer, start_addr+tail, new_msb->buffer_size);
    } else {
      memcpy(new_msb->buffer, start_addr+tail, (metadata_page->data_size - tail));
      memcpy((char*)new_msb->buffer + (metadata_page->data_size - tail), start_addr, head);
    }
    pthread_mutex_lock(&msb_lock);
    new_msb->next = msb;
    msb = new_msb;
    pthread_mutex_unlock(&msb_lock);
    
    metadata_page->data_tail = head;
}

void free_mem_sampling_backed(struct mem_sampling_backed* to_clean)
{
  struct mem_sampling_backed* current = NULL;
  while (to_clean != NULL)
  {
    current = to_clean;
    to_clean = to_clean->next;
    free(current->buffer);
    free(current);
  }
}

int numap_sampling_print_backed(struct numap_sampling_measure *measure, struct mem_sampling_backed *msb, char print_samples) {
  int thread;
  if (msb == NULL) return 0;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    struct mem_sampling_backed* current_msb = msb;
    int fd = measure->fd_per_tid[thread];

    struct perf_event_header *header;
    uint64_t consumed = 0;
    int na_miss_count = 0;
    int cache1_count = 0;
    int cache2_count = 0;
    int cache3_count = 0;
    int lfb_count = 0;
    int memory_count = 0;
    int remote_memory_count = 0;
    int remote_cache_count = 0;
    int total_count = 0;
    while (current_msb != NULL) {
      header  = (struct perf_event_header *)(current_msb->buffer);
      int head = 0;
      if (current_msb->fd == fd) {
        while (head < current_msb->buffer_size) {
          if (header->size == 0) {
            fprintf(stderr, "Error: invalid header size = 0\n");
            return -1;
          }
          if (header -> type == PERF_RECORD_SAMPLE) {
            struct sample *sample = (struct sample *)((char *)(header) + 8);
            if (is_served_by_local_NA_miss(sample->data_src)) {
              na_miss_count++;
            }
            if (is_served_by_local_cache1(sample->data_src)) {
              cache1_count++;
            }
            if (is_served_by_local_cache2(sample->data_src)) {
              cache2_count++;
            }
            if (is_served_by_local_cache3(sample->data_src)) {
              cache3_count++;
            }
            if (is_served_by_local_lfb(sample->data_src)) {
              lfb_count++;
            }
              if (is_served_by_local_memory(sample->data_src)) {
              memory_count++;
            }
            if (is_served_by_remote_memory(sample->data_src)) {
              remote_memory_count++;
            }
            if (is_served_by_remote_cache_or_local_memory(sample->data_src)) {
              remote_cache_count++;
            }
            total_count++;
            if (print_samples) {
              printf("pc=%" PRIx64 ", @=%" PRIx64 ", src level=%s, latency=%" PRIu64 "\n", sample->ip, sample->addr, get_data_src_level(sample->data_src), sample->weight);
            }
          }
          head += header->size;
          header = (struct perf_event_header *)((char*)current_msb->buffer+head);
        }
      }
      current_msb = current_msb->next;
    }
    printf("\n");
    //printf("head = %" PRIu64 " compared to max = %zu\n", head, measure->mmap_len);
    printf("Thread %d: %-8d samples\n", thread, total_count);
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache1_count, "local cache 1", (100.0 * cache1_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache2_count, "local cache 2", (100.0 * cache2_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache3_count, "local cache 3", (100.0 * cache3_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, lfb_count, "local cache LFB", (100.0 * lfb_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, memory_count, "local memory", (100.0 * memory_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, remote_cache_count, "remote cache or local memory", (100.0 * remote_cache_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, remote_memory_count, "remote memory", (100.0 * remote_memory_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, na_miss_count, "unknown l3 miss", (100.0 * na_miss_count / total_count));
  }

  return 0;
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

  // set data backup
  if (numap_sampling_set_mode_buffer_flush(&sm, handler) != 0)
  {
    fprintf(stderr, "numap_sampling_set_mode_buffer_flush error : %s\n", numap_error_message(res));
    return -1;
  }

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
  //numap_sampling_read_print(&sm, 0);
  numap_sampling_print_backed(&sm, msb, 00);
  free_mem_sampling_backed(msb);
  msb = NULL;

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
  if (numap_sampling_set_mode_buffer_flush(&sm, handler) != 0)
  {
    fprintf(stderr, "numap_sampling_set_mode_buffer_flush error : %s\n", numap_error_message(res));
    return -1;
  }
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
  //numap_sampling_write_print(&sm, 0);
  numap_sampling_print_backed(&sm, msb, 0);
  free_mem_sampling_backed(msb);
  msb = NULL;

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
