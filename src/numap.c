#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <err.h>
#include <time.h>
#include <numa.h>
#include <linux/version.h>
#include <pthread.h>

#include "numap.h"

#define PERF_EVENT_MLOCK_KB_FILE "/proc/sys/kernel/perf_event_mlock_kb"

#define NOT_SUPPORTED "NOT_SUPPORTED"

struct archi {
  unsigned int id;
  char name[256];
  char sampling_read_event[256];
  char sampling_write_event[256];
  char counting_read_event[256];
  char counting_write_event[256];
};

/* If your Intel CPU is not supported by Numap, you need to add a new architecture:
 * .id can be found by running lscpu (CPU Familly | Model <<8)
 * .sampling_read_event and .sampling_write_event can be found in the Intel Software Developpers Manuel (Chapter 19)
 */


#define CPU_MODEL(family, model) ((family) | (model) <<8)

static void get_archi(unsigned int archi_id, struct archi * arch) {
  arch->id=archi_id;

  snprintf(arch->name, 256, "Unknown architecture");
  snprintf(arch->sampling_read_event, 256, NOT_SUPPORTED);
  snprintf(arch->sampling_write_event, 256, NOT_SUPPORTED);
  snprintf(arch->counting_read_event, 256, NOT_SUPPORTED);
  snprintf(arch->counting_write_event, 256, NOT_SUPPORTED);

  switch (archi_id)  {
  case CPU_MODEL(6, 151):
  case CPU_MODEL(6, 154):
    snprintf(arch->name, 256, "Alder Lake micro arch");
    /* Not tested. Let's assume these events are the same as the previous cpu generation */
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 167):
    snprintf(arch->name, 256, "Rocket Lake micro arch");
    /* Not tested. Let's assume these events are the same as the previous cpu generation */
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 141):
  case CPU_MODEL(6, 140):
    snprintf(arch->name, 256, "Tiger Lake micro arch");
    /* Not tested. Let's assume these events are the same as the previous cpu generation */
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 125):
  case CPU_MODEL(6, 126):
    snprintf(arch->name, 256, "Ice Lake micro arch");
    /* Not tested. Let's assume these events are the same as the previous cpu generation */
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 102):
    snprintf(arch->name, 256, "Cannon Lake micro arch");
    /* Not tested. Let's assume these events are the same as the previous cpu generation */
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 158):
  case CPU_MODEL(6, 142):
    snprintf(arch->name, 256, "Kaby Lake micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 94):
  case CPU_MODEL(6, 78):
  case CPU_MODEL(6, 85):
    snprintf(arch->name, 256, "Skylake micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_UOPS_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 79):
  case CPU_MODEL(6, 86):
  case CPU_MODEL(6, 71):
  case CPU_MODEL(6, 61):
    snprintf(arch->name, 256, "Broadwell micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_UOPS_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 60):
  case CPU_MODEL(6, 63):
  case CPU_MODEL(6, 69):
  case CPU_MODEL(6, 70):
    snprintf(arch->name, 256, "Haswell micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_UOPS_RETIRED:ALL_STORES");
    break;

  case CPU_MODEL(6, 58):
  case CPU_MODEL(6, 62):
    snprintf(arch->name, 256, "Ivy Bridge micro arch");
    // NOTE: in the Intel SDM, read sampling event is MEM_TRANS_RETIRED:LOAD_LATENCY.
    // In practice this event does not work. As a consequence we use the event below
    // which is the one used by perf mem record and reported by the pfm library
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_TRANS_RETIRED:PRECISE_STORE");
    //snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    //snprintf(arch->sampling_write_event, 256, "MEM_TRANS_RETIRED:PRECISE_STORE");
    break;

  case CPU_MODEL(6, 42):
  case CPU_MODEL(6, 45):
    snprintf(arch->name, 256, "Sandy Bridge micro arch");
  // NOTE: in the Intel SDM, read sampling event is MEM_TRANS_RETIRED:LOAD_LATENCY.
  // In practice this event does not work. As a consequence we use the event below
  // which is the one used by perf mem record and reported by the pfm library
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_TRANS_RETIRED:PRECISE_STORE");
    break;

  case CPU_MODEL(6,46):
  case CPU_MODEL(6,30):
  case CPU_MODEL(6,26):
  case CPU_MODEL(6,31):
    snprintf(arch->name, 256, "Nehalem micro arch");
    snprintf(arch->sampling_read_event, 256,  "MEM_INST_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3");
    break;

  case CPU_MODEL(6, 44):
  case CPU_MODEL(6, 47):
  case CPU_MODEL(6, 37):
    snprintf(arch->name, 256, "Westmere micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_INST_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3");
    break;


    // Intel Xeon Phi CPUs (currently not supported)
    // I'm not sure if PEBS is available on these cpus
  case CPU_MODEL(6, 133):
    snprintf(arch->name, 256, "Knights Mill micro arch");
    break;

  case CPU_MODEL(6, 87):
    snprintf(arch->name, 256, "Knights Landing micro arch");
    break;

    // Old Intel Xeon Phi CPUs
  case CPU_MODEL(11, 0):
    snprintf(arch->name, 256, "Knights Ferry micro arch");
    break;   
  case CPU_MODEL(11, 1):
    snprintf(arch->name, 256, "Knights Corner micro arch");
    break;

    // Some old Intel arch (will probably never be supported)
  case CPU_MODEL(15, 6):
    snprintf(arch->name, 256, "Netburst micro arch");
    break;

  case CPU_MODEL(15, 4):
  case CPU_MODEL(15, 3):
    snprintf(arch->name, 256, "Prescott micro arch");
    break;

  case CPU_MODEL(15, 2):
    snprintf(arch->name, 256, "Northwood micro arch");
    break;
  case CPU_MODEL(15, 1):
    snprintf(arch->name, 256, "Willamette micro arch");
    break;

  case CPU_MODEL(6, 29):
  case CPU_MODEL(6, 23):
    snprintf(arch->name, 256, "Penryn micro arch");
    break;
    
  case CPU_MODEL(6, 15):
  case CPU_MODEL(6, 22):
    snprintf(arch->name, 256, "Core micro arch");
    break;
    
  case CPU_MODEL(6, 14):
    snprintf(arch->name, 256, "Modified Pentium M micro arch");
    break;
    
  case CPU_MODEL(6, 21):
  case CPU_MODEL(6, 13):
  case CPU_MODEL(6, 9):
    snprintf(arch->name, 256, "Pentium M micro arch");
    break;

    // Intel Atom/ Celeron CPUs (will probably never be supported)
  case CPU_MODEL(6, 134):
    snprintf(arch->name, 256, "Tremont micro arch");
    break;
    
  case CPU_MODEL(6, 122):
    snprintf(arch->name, 256, "Goldmont Plus micro arch");
    break;
    
  case CPU_MODEL(6, 95):
  case CPU_MODEL(6, 92):
    snprintf(arch->name, 256, "Goldmont micro arch");
    break;
    
  case CPU_MODEL(6, 76):
    snprintf(arch->name, 256, "Airmont micro arch");
    break;
    
  case CPU_MODEL(6, 55):
  case CPU_MODEL(6, 74):
  case CPU_MODEL(6, 77):
  case CPU_MODEL(6, 93):
    snprintf(arch->name, 256, "Silvermont micro arch");
    break;

  case CPU_MODEL(6, 39):
  case CPU_MODEL(6, 53):
  case CPU_MODEL(6, 54):
    snprintf(arch->name, 256, "Saltwell micro arch");
    break;
    
  case CPU_MODEL(6, 28):
  case CPU_MODEL(6, 38):
    snprintf(arch->name, 256, "Bonnell micro arch");
    break;
  }
}

unsigned char get_family(unsigned int archi_id) {
  return (archi_id >> (8*0)) & 0xff;
}

unsigned char get_model(unsigned int archi_id) {
  return (archi_id >> (8*1)) & 0xff;
}

/**
 * Globals in a shared lib are handled in such a way that each process
 * mapping the lib has its own copy of these globals.
 */
unsigned int nb_numa_nodes;
int numa_node_to_cpu[MAX_NB_NUMA_NODES];
unsigned int perf_event_mlock_kb;
struct archi *current_archi;
char *model_name = NULL;
int curr_err;

struct link_fd_measure {
  struct link_fd_measure *next;
  int fd;
  struct numap_sampling_measure* measure;
};

struct link_fd_measure *link_fd_measure = NULL;
pthread_mutex_t link_fd_lock;

/**
 * Special function called each time a process using the lib is
 * started.
 */
__attribute__((constructor)) void init(void) {

  int node;
  int cpu;

  // Supported_archs

  // Check architecture
  FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
  char *arg = NULL;
  size_t size = 0;
  unsigned char family = 0;
  unsigned char model = 0;
  const char *family_string = "cpu family\t";
  const char *model_string = "model\t";
  const char *model_name_string = "model name\t";
  while(getline(&arg, &size, cpuinfo) != -1 && (!family || !model || !model_name)) {
    if (strncmp(family_string, arg, strlen(family_string)) == 0) {
      strtok(arg, ":");
      family = atoi(strtok(NULL, "\n"));
    } else if (strncmp(model_string, arg, strlen(model_string)) == 0) {
      strtok(arg, ":");
      model = atoi(strtok(NULL, "\n"));
    } else if (strncmp(model_name_string, arg, strlen(model_name_string)) == 0) {
      strtok(arg, ":");
      char * model_name_strtok = strtok(NULL, "\n");
      int len = strlen(model_name_strtok);
      model_name = malloc((len + 1) * sizeof(char));
      strcpy(model_name, model_name_strtok);
    }
  }
  free(arg);
  fclose(cpuinfo);
  current_archi = malloc(sizeof(struct archi));
  get_archi(CPU_MODEL(family, model), current_archi);

  // Get numa configuration
  int available = numa_available();
  if (available == -1) {
    nb_numa_nodes = -1;
  } else {
    nb_numa_nodes = numa_num_configured_nodes();
    int nb_cpus = numa_num_configured_cpus();
    for (node = 0; node < nb_numa_nodes; node++) {
      struct bitmask *mask = numa_allocate_cpumask();
      numa_node_to_cpus(node, mask);
      numa_node_to_cpu[node] = -1;
      for (cpu = 0; cpu < nb_cpus; cpu++) {
        if (*(mask->maskp) & (1 << cpu)) {
          numa_node_to_cpu[node] = cpu;
          break;
        }
      }
      numa_bitmask_free(mask);
    }
  }

  // Get perf config
  FILE *f = fopen(PERF_EVENT_MLOCK_KB_FILE, "r");
  if (!f) {
    fprintf(stderr, "Failed to open file: `%s'\n", PERF_EVENT_MLOCK_KB_FILE);
    exit(-1);
  }
  if (fscanf(f, "%u", &perf_event_mlock_kb) != 1) {
    fprintf(stderr, "Failed to read perf_event_mlock_kb from file: `%s'\n", PERF_EVENT_MLOCK_KB_FILE);
    fclose(f);
    exit(-1);
  }
  fclose(f);
}

char* concat(const char *s1, const char *s2) {
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  if (result == NULL) {
    return "malloc failed in concat\n";
  }
  strcpy(result, s1);
  strcat(result, s2);
 return result;
}

char* build_string(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char* result = NULL;
  vasprintf(&result, fmt, args);
  return result;
}
 
const char *numap_error_message(int error) {
  switch (error) {
  case ERROR_NUMAP_NOT_NUMA:
    return "libnumap: numa lib not available";
  case ERROR_NUMAP_STOP_BEFORE_START:
    return "libnumap: stop called before start";
  case ERROR_NUMAP_ALREADY_STARTED:
    return "libnumap: start called again before stop";
  case ERROR_NUMAP_ARCH_NOT_SUPPORTED:
    return build_string("libnumap: architecture not supported: %s (family %d, model %d)",
          model_name, get_family(current_archi->id), get_model(current_archi->id));
  case ERROR_NUMAP_READ_SAMPLING_ARCH_NOT_SUPPORTED:
    return build_string("libnumap: read sampling not supported on architecture: %s (family %d, model %d)",
          model_name, get_family(current_archi->id), get_model(current_archi->id));
  case ERROR_NUMAP_WRITE_SAMPLING_ARCH_NOT_SUPPORTED:
    return build_string("libnumap: write sampling not supported on architecture: %s (family %d, model %d)",
          model_name, get_family(current_archi->id), get_model(current_archi->id));
  case ERROR_PERF_EVENT_OPEN:
    return build_string("libnumap: error when calling perf_event_open: %s", strerror(errno));
  case ERROR_PFM:
    return build_string("libnumap: error when initializing pfm: %s", pfm_strerror(curr_err));
  case ERROR_READ:
    return "libnumap: error while trying to read counter";
  default:
    return "libnumap: unknown error";
  }
}

int set_signal_handler(void(*handler)(int, siginfo_t*,void*)) {
  struct sigaction sigoverflow;
  memset(&sigoverflow, 0, sizeof(struct sigaction));
  sigoverflow.sa_sigaction = handler;
  sigoverflow.sa_flags = SA_SIGINFO;

  if (sigaction(SIGIO, &sigoverflow, NULL) < 0)
  {
    fprintf(stderr, "could not set up signal handler\n");
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int numap_init(void) {

  if (nb_numa_nodes == -1) {
    return ERROR_NUMAP_NOT_NUMA;
  }

  if (current_archi == NULL) {
    return ERROR_NUMAP_ARCH_NOT_SUPPORTED;
  }

  curr_err = pfm_initialize();
  if (curr_err != PFM_SUCCESS) {
    return ERROR_PFM;
  }

  link_fd_measure = NULL;
  pthread_mutex_init(&link_fd_lock, NULL);

  return 0;
}

int numap_counting_init_measure(struct numap_counting_measure *measure) {

  measure->nb_nodes = nb_numa_nodes;
  for (int node = 0; node < nb_numa_nodes; node++) {
    measure->is_valid[node] = (numa_node_to_cpu[node] != -1);
  }
  measure->started = 0;
  return 0;
}

void refresh_wrapper_handler(int signum, siginfo_t *info, void* ucontext) {
  if (info->si_code == POLL_HUP) {
    /* TODO: copy the samples */

    int fd = info->si_fd;

    // search for corresponding measure
    struct link_fd_measure* current_lfm = link_fd_measure;
    while (current_lfm != NULL && current_lfm->fd != fd) {
      current_lfm = current_lfm->next;
    }
    if (current_lfm == NULL)
    {
      fprintf(stderr, "No measure associated with fd %d\n", fd);
      abort();
      exit(EXIT_FAILURE);
    }
    struct numap_sampling_measure* measure = current_lfm->measure;

    if (measure->handler) {
      measure->handler(measure, fd);
    }
    measure->total_samples += measure->nb_refresh;

    ioctl(info->si_fd, PERF_EVENT_IOC_REFRESH, measure->nb_refresh);
  }
}

int numap_sampling_set_measure_handler(struct numap_sampling_measure *measure, void(*handler)(struct numap_sampling_measure*,int), int nb_refresh)
{
  // Has to be called before the measure starts
  if (measure->started != 0)
  {
    return ERROR_NUMAP_ALREADY_STARTED;
  }
  // Set signal handler
  // may be given as function argument later
  measure->handler = handler;
  if (nb_refresh <= 0)
  {
	  fprintf(stderr, "Undefined behaviour : nb_refresh %d <= 0\n", nb_refresh);
	  exit(EXIT_FAILURE);
  }
  measure->nb_refresh = nb_refresh;

  return 0;
}

int __numap_counting_start(struct numap_counting_measure *measure, struct perf_event_attr *pe_attr_read, struct perf_event_attr *pe_attr_write) {

  /**
   * Check everything is ok
   */
  if (measure->started != 0) {
    return ERROR_NUMAP_ALREADY_STARTED;
  } else {
    measure->started++;
  }

  // Open the events on each NUMA node with Linux system call
  for (int node = 0; node < measure->nb_nodes; node++) {
    if (measure->is_valid[node]) {
      measure->fd_reads[node] = perf_event_open(pe_attr_read, -1, numa_node_to_cpu[node], -1, 0);
      if (measure->fd_reads[node] == -1) {
        return ERROR_PERF_EVENT_OPEN;
      }
      measure->fd_writes[node] = perf_event_open(pe_attr_write, -1, numa_node_to_cpu[node], -1, 0);
      if (measure->fd_writes[node] == -1) {
        return ERROR_PERF_EVENT_OPEN;
      }
    }
  }

  // Starts measure
  for (int node = 0; node < measure->nb_nodes; node++) {
    if (measure->is_valid[node]) {
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_ENABLE, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_ENABLE, 0);
    }
  }

  return 0;
}

int numap_counting_start(struct numap_counting_measure *measure) {

  // Set attribute parameter for counting reads using pfmlib
  struct perf_event_attr pe_attr_read;
  memset(&pe_attr_read, 0, sizeof(pe_attr_read));
  pe_attr_read.size = sizeof(pe_attr_read);
  pfm_perf_encode_arg_t arg;
  memset(&arg, 0, sizeof(arg));
  arg.size = sizeof(pfm_perf_encode_arg_t);
  arg.attr = &pe_attr_read;
  char *fstr;
  arg.fstr = &fstr;
  curr_err = pfm_get_os_event_encoding(current_archi->counting_read_event, PFM_PLM0 | PFM_PLM3, PFM_OS_PERF_EVENT, &arg);
  if (curr_err != PFM_SUCCESS) {
    return ERROR_PFM;
  }

  // Set attribute parameter for counting writes using pfmlib
  struct perf_event_attr pe_attr_write;

  // Other parameters
  pe_attr_read.disabled = 1;
  pe_attr_read.exclude_kernel = 1;
  pe_attr_read.exclude_hv = 1;

  return __numap_counting_start(measure, &pe_attr_read, &pe_attr_write);
}

int numap_counting_stop(struct numap_counting_measure *measure) {

  // Check everything is ok
  if (measure->started == 0) {
    return ERROR_NUMAP_STOP_BEFORE_START;
  } else {
    measure->started = 0;
  }

  for (int node = 0; node < nb_numa_nodes; node++) {
    if (measure->is_valid[node]) {
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_DISABLE, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_DISABLE, 0);
      if(read(measure->fd_reads[node], &measure->reads_count[node],
            sizeof(long long)) == -1) {
        return ERROR_READ;
      }
      if (read(measure->fd_writes[node], &measure->writes_count[node],
            sizeof(long long)) == -1) {
        return ERROR_READ;
      }
      close(measure->fd_reads[node]);
      close(measure->fd_writes[node]);
    }
  }

  return 0;
}

int numap_sampling_init_measure(struct numap_sampling_measure *measure, int nb_threads, int sampling_rate, int mmap_pages_count) {

  int thread;
  measure->started = 0;
  measure->page_size = (size_t)sysconf(_SC_PAGESIZE);
  measure->mmap_pages_count = mmap_pages_count;
  measure->mmap_len = measure->page_size + measure->page_size * measure->mmap_pages_count;
  measure->nb_threads = nb_threads;
  measure->sampling_rate = sampling_rate;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    measure->fd_per_tid[thread] = 0;
    measure->metadata_pages_per_tid[thread] = 0;
  }
  measure->handler = NULL;
  measure->total_samples = 0;
  set_signal_handler(refresh_wrapper_handler);
  measure->nb_refresh = 1000; // default refresh 
 
  return 0;
}


static int __numap_sampling_resume(struct numap_sampling_measure *measure) {
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_RESET, 0);
    fcntl(measure->fd_per_tid[thread], F_SETFL, O_ASYNC|O_NONBLOCK);
    fcntl(measure->fd_per_tid[thread], F_SETSIG, SIGIO);
    fcntl(measure->fd_per_tid[thread], F_SETOWN, measure->tids[thread]);
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_REFRESH, measure->nb_refresh);
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_ENABLE, 0);
  }
 return 0;
}

int numap_sampling_resume(struct numap_sampling_measure *measure) {
  /**
   * Check everything is ok
   */
  if (measure->started != 0) {
    return ERROR_NUMAP_ALREADY_STARTED;
  } else {
    measure->started++;
  }

  return __numap_sampling_resume(measure);
}

int __numap_sampling_start(struct numap_sampling_measure *measure, struct perf_event_attr *pe_attr) {

  /**
   * Check everything is ok
   */
  if (measure->started != 0) {
    return ERROR_NUMAP_ALREADY_STARTED;
  } else {
    measure->started++;
  }

  /**
   * Open the events for each thread in measure with Linux system call: we do per
   * thread monitoring by giving the system call the thread id and a
   * cpu = -1, this way the kernel handles the migration of counters
   * when threads are migrated. Then we mmap the result.
   */
  int cpu = -1;
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    if(measure->metadata_pages_per_tid[thread]) {
      /* Already open, we can skip this one */
      continue;
    }

    measure->fd_per_tid[thread] = perf_event_open(pe_attr, measure->tids[thread], cpu,
                          -1, 0);
    if (measure->fd_per_tid[thread] == -1) {
      return ERROR_PERF_EVENT_OPEN;
    }
    measure->metadata_pages_per_tid[thread] = mmap(NULL, measure->mmap_len, PROT_WRITE, MAP_SHARED, measure->fd_per_tid[thread], 0);
    if (measure->metadata_pages_per_tid[thread] == MAP_FAILED) {
      if (errno == EPERM) {
        fprintf(stderr, "Permission error mapping pages.\n"
        "Consider increasing /proc/sys/kernel/perf_event_mlock_kb,\n"
        "(mmap length parameter = %zd > perf_event_mlock_kb = %u)\n", measure->mmap_len, (perf_event_mlock_kb * 1024));
      } else {
        fprintf (stderr, "Couldn't mmap file descriptor: %s - errno = %d\n",
        strerror (errno), errno);
      }
      exit (EXIT_FAILURE);
    }
    struct link_fd_measure* new_lfm = malloc(sizeof(struct link_fd_measure));
    new_lfm->fd = measure->fd_per_tid[thread];
    new_lfm->measure = measure;

    pthread_mutex_lock(&link_fd_lock);
    new_lfm->next = link_fd_measure;
    link_fd_measure = new_lfm;
    pthread_mutex_unlock(&link_fd_lock);
  }
  __numap_sampling_resume(measure);
  
  return 0;
}

int numap_sampling_read_supported() {

  if (strcmp(current_archi->sampling_read_event, NOT_SUPPORTED) == 0) {
    return 0;
  }
  return 1;
}

int numap_sampling_read_start_generic(struct numap_sampling_measure *measure, uint64_t sample_type) {

  // Checks that read sampling is supported before calling pfm
  if (!numap_sampling_read_supported()) {
    return ERROR_NUMAP_READ_SAMPLING_ARCH_NOT_SUPPORTED;
  }
  
  // Set attribute parameter for perf_event_open using pfmlib
  struct perf_event_attr pe_attr;
  memset(&pe_attr, 0, sizeof(pe_attr));
  pe_attr.size = sizeof(pe_attr);
  pfm_perf_encode_arg_t arg;
  memset(&arg, 0, sizeof(arg));
  arg.size = sizeof(pfm_perf_encode_arg_t);
  arg.attr = &pe_attr;
  char *fstr;
  arg.fstr = &fstr;

  curr_err = pfm_get_os_event_encoding(current_archi->sampling_read_event, PFM_PLM0 | PFM_PLM3, PFM_OS_PERF_EVENT, &arg);
  if (curr_err != PFM_SUCCESS) {
    return ERROR_PFM;
  }

  // Sampling parameters
  pe_attr.sample_period = measure->sampling_rate;
  pe_attr.sample_type = sample_type;
  pe_attr.mmap = 1;
  pe_attr.task = 1;
  pe_attr.precise_ip = 2;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
  pe_attr.use_clockid=1;
  pe_attr.clockid = CLOCK_MONOTONIC_RAW;
#else
#warning NUMAP: using clockid is not possible on kernel version < 4.1. This feature will be disabled.
#endif
  // Other parameters
  pe_attr.disabled = 1;
  pe_attr.exclude_kernel = 1;
  pe_attr.exclude_hv = 1;

  return __numap_sampling_start(measure, &pe_attr);

}
  
int numap_sampling_read_start(struct numap_sampling_measure *measure) {
  return numap_sampling_read_start_generic(measure, PERF_SAMPLE_IP | PERF_SAMPLE_ADDR | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC);
}

int numap_sampling_read_stop(struct numap_sampling_measure *measure) {

  // Check everything is ok
  if (measure->started == 0) {
    return ERROR_NUMAP_STOP_BEFORE_START;
  } else {
    measure->started = 0;
  }
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_DISABLE, 0);
  }
  return 0;
}

int numap_sampling_write_supported() {

  if (strcmp(current_archi->sampling_write_event, NOT_SUPPORTED) == 0) {
    return 0;
  }
  return 1;
}

int numap_sampling_write_start_generic(struct numap_sampling_measure *measure, uint64_t sample_type) {
  // Checks that write sampling is supported before calling pfm
  if (!numap_sampling_write_supported()) {
    return ERROR_NUMAP_WRITE_SAMPLING_ARCH_NOT_SUPPORTED;
  }

  // Set attribute parameter for perf_event_open using pfmlib
  struct perf_event_attr pe_attr;
  memset(&pe_attr, 0, sizeof(pe_attr));
  pe_attr.size = sizeof(pe_attr);
  pfm_perf_encode_arg_t arg;
  memset(&arg, 0, sizeof(arg));
  arg.size = sizeof(pfm_perf_encode_arg_t);
  arg.attr = &pe_attr;
  char *fstr;
  arg.fstr = &fstr;  
  curr_err = pfm_get_os_event_encoding(current_archi->sampling_write_event, PFM_PLM0 | PFM_PLM3, PFM_OS_PERF_EVENT, &arg);
  if (curr_err != PFM_SUCCESS) {
    return ERROR_PFM;
  }

  // Sampling parameters
  pe_attr.sample_period = measure->sampling_rate;
  pe_attr.sample_type = sample_type;
  pe_attr.mmap = 1;
  pe_attr.task = 1;
  pe_attr.precise_ip = 2;

  // Other parameters
  pe_attr.disabled = 1;
  pe_attr.exclude_kernel = 1;
  pe_attr.exclude_hv = 1;

  return __numap_sampling_start(measure, &pe_attr);
}


int numap_sampling_write_start(struct numap_sampling_measure *measure) {
  return numap_sampling_write_start_generic(measure, PERF_SAMPLE_IP | PERF_SAMPLE_ADDR | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC);
}

int numap_sampling_write_stop(struct numap_sampling_measure *measure) {
  return numap_sampling_read_stop(measure);
}

int numap_sampling_end(struct numap_sampling_measure *measure) {
  int thread;

  struct link_fd_measure* current_lfm;
  while (link_fd_measure != NULL) {
    current_lfm = link_fd_measure;
    link_fd_measure = link_fd_measure->next;
    free(current_lfm);
  }

  for (thread = 0; thread < measure->nb_threads; thread++) {
    munmap(measure->metadata_pages_per_tid[thread], measure->mmap_len);
    close(measure->fd_per_tid[thread]);
  }
  return 0;
}
