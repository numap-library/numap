#include <inttypes.h>
#include <sys/types.h>
#include <perfmon/pfmlib_perf_event.h>
#include <signal.h>

#define MAX_NB_NUMA_NODES     16
#define MAX_NB_THREADS        24
#define MAX_NB_CPUS           32

#define ERROR_PERF_EVENT_OPEN  		              -3
#define ERROR_NUMAP_NOT_NUMA  			      -4
#define ERROR_NUMAP_ALREADY_STARTED	              -5
#define ERROR_NUMAP_STOP_BEFORE_START                 -6
#define ERROR_NUMAP_ARCH_NOT_SUPPORTED                -7
#define ERROR_NUMAP_READ_SAMPLING_ARCH_NOT_SUPPORTED  -9
#define ERROR_NUMAP_WRITE_SAMPLING_ARCH_NOT_SUPPORTED -10
#define ERROR_PFM                                     -11
#define ERROR_READ                                    -12

#define rmb()		asm volatile("lfence" ::: "memory")

/**
 * Structure representing a measurement of counting the load of controlers.
 */
struct numap_counting_measure {
  char started;
  int nb_nodes;
  long fd_reads[MAX_NB_NUMA_NODES];
  long fd_writes[MAX_NB_NUMA_NODES];
  long long reads_count[MAX_NB_NUMA_NODES];
  long long writes_count[MAX_NB_NUMA_NODES];
};

/**
 * Structure representing a measurement of memory read or write sampling.
 */
struct numap_sampling_measure {

  /*
   * Fields to be written and/or read by user code.
   */
  unsigned int nb_threads;
  unsigned int sampling_rate;
  unsigned int mmap_pages_count; // Must be power of two as specified in the man page of perf_event_open (mmap size should be 1+2^n pages)
  pid_t tids[MAX_NB_THREADS];
  struct perf_event_mmap_page *metadata_pages_per_tid[MAX_NB_THREADS];

  /*
   * Fields to be written and/or read by library code.
   */
  size_t page_size;
  size_t mmap_len;
  char started;
  long fd_per_tid[MAX_NB_THREADS];
  char buffer_flush_enabled;
};

/**
 * Structure representing a read sample gathered with the library in
 * sampling mode.
 */
struct __attribute__ ((__packed__)) sample {
  uint64_t ip;
  uint64_t addr;
  uint64_t weight;
  union perf_mem_data_src data_src;
};

/**
 * Structure representing a mmap sample gathered with the library in
 * sampling mode.
 */
struct __attribute__ ((__packed__)) mmap_sample {
  uint32_t pid;
  uint32_t tid;
  uint64_t addr;
  uint64_t len;
  uint64_t pgoff;
  char filename[100];
};

char* concat(const char *s1, const char *s2);

/**
 * Numap initialization function
 */
int numap_init(void);

/**
 * Memory counting.
 */
int numap_counting_init_measure(struct numap_counting_measure *measure);
int numap_counting_start(struct numap_counting_measure *measure);
int numap_counting_stop(struct numap_counting_measure *measure);

/**
 * Memory read and write sampling.
 */
void perf_overflow_handler(int, siginfo_t*, void*);
int numap_sampling_set_mode_buffer_flush(struct numap_sampling_measure *measure, void(*)(int,siginfo_t*,void*));
int numap_sampling_init_measure(struct numap_sampling_measure *measure, int nb_threads, int sampling_rate, int mmap_pages_count);
int numap_sampling_read_start_generic(struct numap_sampling_measure *measure, uint64_t sample_type);
int numap_sampling_read_start(struct numap_sampling_measure *measure);
int numap_sampling_read_stop(struct numap_sampling_measure *measure);
int numap_sampling_read_print(struct numap_sampling_measure *measure, char print_samples);
int numap_sampling_write_supported();
int numap_sampling_write_start_generic(struct numap_sampling_measure *measure, uint64_t sample_type);
int numap_sampling_write_start(struct numap_sampling_measure *measure);
int numap_sampling_write_stop(struct numap_sampling_measure *measure);
int numap_sampling_write_print(struct numap_sampling_measure *measure, char print_samples);
int numap_sampling_print(struct numap_sampling_measure *measure, char print_samples);
int numap_sampling_end(struct numap_sampling_measure *measure);
int numap_sampling_resume(struct numap_sampling_measure *measure);

/**
 * Error handling.
 */
const char *numap_error_message(int error);

/**
 * Functions to analyse results
 */
int is_served_by_local_cache1(union perf_mem_data_src data_src);
int is_served_by_local_cache2(union perf_mem_data_src data_src);
int is_served_by_local_cache3(union perf_mem_data_src data_src);
int is_served_by_local_lfb(union perf_mem_data_src data_src);
int is_served_by_local_cache(union perf_mem_data_src data_src);
int is_served_by_local_memory(union perf_mem_data_src data_src);
int is_served_by_remote_cache_or_local_memory(union perf_mem_data_src data_src);
int is_served_by_remote_memory(union perf_mem_data_src data_src);
int is_served_by_local_NA_miss(union perf_mem_data_src data_src);
char *get_data_src_opcode(union perf_mem_data_src data_src);
char *get_data_src_level(union perf_mem_data_src data_src);
