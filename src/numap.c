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
#include <err.h>
#include <numa.h>

#include "numap.h"

#define PERF_EVENT_MLOCK_KB_FILE "/proc/sys/kernel/perf_event_mlock_kb"

struct archi {
  unsigned int id;
  const char *name;
  const char *sampling_read_event;
  const char *sampling_write_event;
};

#define NB_SUPPORTED_ARCHS 9

struct archi Xeon_E_7450 = { .id = 0x06 | 0x1D << 8, // 06_29
			     .name = "Xeon_E_7450 based on Penryn micro arch - Dunnington decline",
			     .sampling_read_event= "NOT_SUPPORTED",
			     .sampling_write_event="NOT_SUPPORTED"
};

struct archi I7_870 = { .id = 0x06 | 0x1E << 8, // 06_30
			.name = "I7_870 based on Nehalem micro arch - Lynfield decline",
			.sampling_read_event= "MEM_INST_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3",
			.sampling_write_event="NOT_SUPPORTED"
};

struct archi WESTMERE_EP = { .id = 0x06 | 0x2C << 8, // O6_44
			     .name = "Westmere-Ep",
			     .sampling_read_event= "MEM_INST_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3",
			     .sampling_write_event="NOT_SUPPORTED"
};

struct archi Xeon_E5_2670 = { .id = 0x06 | 0x2D << 8, // 06_45
			      .name = "Xeon E5-2670 based on Sandy Bridge micro arch - Sandy Bridge-EP decline",
			      .sampling_read_event= "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3",
			      .sampling_write_event="MEM_TRANS_RETIRED:PRECISE_STORE"
};

struct archi I5_2520 = { .id = 0x06 | 0x2A << 8, // 06_42
			 .name = "I5_2520 based on Sandy Bridge micro arch - Sandy Bridge decline - 2nd generation Intel Core",
			 // NOTE in the Intel SDM it's named MEM_TRANS_RETIRED:LOAD_LATENCY
			 // but the number correspond with  the one returned pfmlib
			 .sampling_read_event= "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3",
			 .sampling_write_event="MEM_TRANS_RETIRED:PRECISE_STORE"
};

struct archi Xeon_E5_2660 = { .id = 0x06 | 0x3E << 8, // 06_62
			 .name = "Xeon_E5_2660 based on Ivy Bridge micro arch - Ivy Bridge-E decline - 3rd generation Intel Core",
			 // NOTE in the Intel SDM it's named MEM_TRANS_RETIRED:LOAD_LATENCY
			 // but the number correspond with  the one returned pfmlib
			 .sampling_read_event= "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3",
			 .sampling_write_event="MEM_TRANS_RETIRED:PRECISE_STORE"
};

struct archi I7_3770 = { .id = 0x06 | 0x3A << 8, // 06_58
			 .name = "I7_3770 based on Ivy Bridge micro arch - Ivy Bridge decline - 3rd generation Intel Core",
			 .sampling_read_event= "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3",
			 .sampling_write_event="MEM_TRANS_RETIRED:PRECISE_STORE"
};

struct archi I7_5960X = { .id = 0x06 | 0x3F << 8, // 06_63
			  .name = "I7_5960X based on Haswell micro arch - Haswell-E decline - 4th generation Intel Core",
			  .sampling_read_event= "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3",
			  .sampling_write_event="MEM_UOPS_RETIRED:ALL_STORES"
};

struct archi I5_4670 = { .id = 0x06 | 0x3C << 8, // 06_60
			  .name = "I5-4670 based on Haswell micro arch - Haswell-DT decline - 4th generation Intel Core",
			  .sampling_read_event= "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3",
			  .sampling_write_event="MEM_UOPS_RETIRED:ALL_STORES"
};


static struct archi *supported_archs[NB_SUPPORTED_ARCHS] = {
  &Xeon_E_7450,
  &I7_870,
  &WESTMERE_EP,
  &Xeon_E5_2670,
  &I5_2520,
  &Xeon_E5_2660,
  &I7_3770,
  &I7_5960X,
  &I5_4670};

static struct archi * get_archi(unsigned int archi_id) {
  int i;
  for (i = 0; i < NB_SUPPORTED_ARCHS; i++) {
    if (archi_id == supported_archs[i]->id) {
      return supported_archs[i];
    }
  }
  return NULL;
}

static unsigned char get_family(unsigned int archi_id) {
  return (archi_id >> (8*0)) & 0xff;
}

static unsigned char get_model(unsigned int archi_id) {
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
  char *arg;
  size_t size = 0;
  unsigned char family, model;
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
      model_name = malloc(len);
      strcpy(model_name, model_name_strtok);
    }
  }
  free(arg);
  fclose(cpuinfo);
  current_archi = get_archi(family | model << 8);
  //fprintf(stderr, "family=%u, model=%u\n", get_family(current_archi->id), get_model(current_archi->id));

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
      if (numa_node_to_cpu[node] == -1) {
	nb_numa_nodes = -1; // to be handled properly
      }
    }
  }
  // printf("Nb NUMA nodes = %d\n", nb_numa_nodes);

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



const char *numap_error_message(int error) {
  char *pe_error = "perf_event ==> ";
  char *pfm_error = "pfm ==> ";
  switch (error) {
  case ERROR_NUMEMP_NOT_NUMA:
    return "libnumap: numa lib not available";
  case ERROR_NUMEMP_STOP_BEFORE_START:
    return "libnumap: stop called before start";
  case ERROR_NUMEMP_ALREADY_STARTED:
    return "libnumap: start called again before stop";
  case ERROR_NUMEMP_ARCH_NOT_SUPPORTED:
    return concat("libnumap: architecture not supported: ", model_name);
  case ERROR_PERF_EVENT_OPEN:
    return concat(pe_error, strerror(errno));
  case ERROR_PFM:
    return concat(pfm_error, pfm_strerror(curr_err));
  case ERROR_READ:
    return "libnumap: error while trying to read counter";
  default:
    return "libnumap: unknown error";
  }
}

int numap_init(void) {

  if (nb_numa_nodes == -1) {
    return ERROR_NUMEMP_NOT_NUMA;
  }

  if (current_archi == NULL) {
    return ERROR_NUMEMP_ARCH_NOT_SUPPORTED;
  }

  curr_err = pfm_initialize();
  if (curr_err != PFM_SUCCESS) {
    return ERROR_PFM;
  }
  return 0;
}

int numap_bdw_init_measure(struct numap_bdw_measure *measure) {

  measure->nb_nodes = nb_numa_nodes;
  measure->started = 0;
  return 0;
}

int numap_bdw_start(struct numap_bdw_measure *measure) {
  int node;

  // Check everything is ok
  if (measure->started != 0) {
    return ERROR_NUMEMP_ALREADY_STARTED;
  } else {
    measure->started++;
  }

  // Bull's Westmere EP
  unsigned int family = get_family(current_archi->id);
  unsigned int model = get_model(current_archi->id);
  if (family == 6 && model == 44) {

    // Manually set 64 bytes cache line reads/writes from memory counting event
    struct perf_event_attr pe_attr_unc_memory;
    memset(&pe_attr_unc_memory, 0, sizeof(pe_attr_unc_memory));
    pe_attr_unc_memory.size = sizeof(pe_attr_unc_memory);
    pe_attr_unc_memory.type = 6; // /sys/bus/event_source/uncore/type
    pe_attr_unc_memory.config = 0x072c; // UNC_QMC_NORMAL_READS.ANY
    pe_attr_unc_memory.disabled = 1;
    struct perf_event_attr pe_attr_unc_write_memory;
    memset(&pe_attr_unc_write_memory, 0, sizeof(pe_attr_unc_write_memory));
    pe_attr_unc_write_memory.size = sizeof(pe_attr_unc_write_memory);
    pe_attr_unc_write_memory.type = 6; // /sys/bus/event_source/uncore/type
    pe_attr_unc_write_memory.config = 0x072f; // UNC_QMC_WRITES.FULL.ANY
    pe_attr_unc_write_memory.disabled = 1;

    // Open the events on each NUMA node with Linux system call
    for (node = 0; node < nb_numa_nodes; node++) {
      measure->fd_reads[node] = perf_event_open(&pe_attr_unc_memory, -1, numa_node_to_cpu[node], -1, 0);
      if (measure->fd_reads[node] == -1) {
	return ERROR_PERF_EVENT_OPEN;
      }
      measure->fd_writes[node] = perf_event_open(&pe_attr_unc_write_memory, -1, numa_node_to_cpu[node], -1, 0);
      if (measure->fd_writes[node] == -1) {
	return ERROR_PERF_EVENT_OPEN;
      }
    }

    // Starts measure
    for (node = 0; node < nb_numa_nodes; node++) {
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_ENABLE, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_ENABLE, 0);
    }
  }

  // Manu's laptop
  // CARE DOESN'COUNT THE GOOD THING, IT SHOULD BE COUNTED ON ALL THE PROCESSOR
  // CARE JUST FOR DEMONSTRATION PURPOSE
  if (family == 6 && model == 42) {

    // Use pfmlib to set offcore request LOAD with response from MEMORY and ANY_SNOOP
    pfm_perf_encode_arg_t arg_load;
    struct perf_event_attr arg_load_pe_attr;
    memset(&arg_load, 0, sizeof(arg_load));
    memset(&arg_load_pe_attr, 0, sizeof(arg_load_pe_attr));
    arg_load.size = sizeof(pfm_perf_encode_arg_t);
    arg_load.attr = &arg_load_pe_attr;
    char *fstr;
    arg_load.fstr = &fstr;
    curr_err = pfm_get_os_event_encoding("OFFCORE_RESPONSE_0:ANY_DATA:LLC_MISS_LOCAL_DRAM:SNP_ANY", PFM_PLM0 | PFM_PLM3, PFM_OS_PERF_EVENT, &arg_load);
    if (curr_err != PFM_SUCCESS) {
      return ERROR_PFM;
    }

    pfm_perf_encode_arg_t arg_store;
    struct perf_event_attr arg_store_pe_attr;
    memset(&arg_store, 0, sizeof(arg_store));
    memset(&arg_store_pe_attr, 0, sizeof(arg_store_pe_attr));
    arg_store.attr = &arg_store_pe_attr;
    arg_store.size = sizeof(pfm_perf_encode_arg_t);
    curr_err = pfm_get_os_event_encoding("OFFCORE_RESPONSE_0:ANY_RFO:LLC_MISS_LOCAL_DRAM:SNP_ANY", PFM_PLM0 | PFM_PLM0, PFM_OS_PERF_EVENT, &arg_store);
    if (curr_err != PFM_SUCCESS) {
      return ERROR_PFM;
    }

    // Open the events on each CPU with Linux system call
    int node = 0; // single node
    int pid = -1;
    int cpu = 0;
    measure->fd_reads[node] = perf_event_open(arg_load.attr, pid, cpu, -1, 0);
    if (measure->fd_reads[node] == -1) {
      return ERROR_PERF_EVENT_OPEN;
    }
    measure->fd_writes[node] = perf_event_open(arg_store.attr, pid, cpu, -1, 0);
    if (measure->fd_writes[node] == -1) {
      return ERROR_PERF_EVENT_OPEN;
    }

    // Starts measure
    for (node = 0; node < nb_numa_nodes; node++) {
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_reads[node], PERF_EVENT_IOC_ENABLE, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_RESET, 0);
      ioctl(measure->fd_writes[node], PERF_EVENT_IOC_ENABLE, 0);
    }
  }

  return 0;
}

int numap_bdw_stop(struct numap_bdw_measure *measure) {
  int node;

  // Check everything is ok
  if (measure->started == 0) {
    return ERROR_NUMEMP_STOP_BEFORE_START;
  } else {
    measure->started = 0;
  }

  for (node = 0; node < nb_numa_nodes; node++) {
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

  return 0;
}

int numap_sampling_init_measure(struct numap_sampling_measure *measure, int nb_threads, int sampling_rate, int mmap_pages_count) {

  measure->started = 0;
  measure->page_size = (size_t)sysconf(_SC_PAGESIZE);
  measure->mmap_pages_count = mmap_pages_count;
  measure->mmap_len = measure->page_size + measure->page_size * measure->mmap_pages_count;
  measure->nb_threads = nb_threads;
  measure->sampling_rate = sampling_rate;

  /* fprintf (stderr, "mmap_len                        = %zu bytes\n", measure->mmap_len); */
  /* fprintf (stderr, "mmap_len - PAGE_SIZE            = %lu bytes\n", measure->mmap_len - measure->page_size); */
  /* fprintf (stderr, "perf_event_mlock_kb             = %lu bytes\n", perf_event_mlock_kb * 1024l); */

  return 0;
}

int __numap_sampling_start(struct numap_sampling_measure *measure, struct perf_event_attr *pe_attr) {

  /**
   * Check everything is ok
   */
  if (measure->started != 0) {
    return ERROR_NUMEMP_ALREADY_STARTED;
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
  }
  for (thread = 0; thread < measure->nb_threads; thread++) {
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_RESET, 0);
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_ENABLE, 0);
  }

  return 0;
}

int numap_sampling_read_start(struct numap_sampling_measure *measure) {
  
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
  pe_attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR | PERF_SAMPLE_WEIGHT | PERF_SAMPLE_DATA_SRC;
  pe_attr.mmap = 1;
  pe_attr.task = 1;
  pe_attr.precise_ip = 2;

  // Other parameters
  pe_attr.disabled = 1;
  pe_attr.exclude_kernel = 1;
  pe_attr.exclude_hv = 1;

  //printf("PE: config = %" PRIx64 ", config1 = %" PRIx64 ", precise = %d\n", pe_attr.config, pe_attr.config1, pe_attr.precise_ip);
  
  return __numap_sampling_start(measure, &pe_attr);
}

int numap_sampling_read_stop(struct numap_sampling_measure *measure) {

  // Check everything is ok
  if (measure->started == 0) {
    return ERROR_NUMEMP_STOP_BEFORE_START;
  } else {
    measure->started = 0;
  }
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    ioctl(measure->fd_per_tid[thread], PERF_EVENT_IOC_DISABLE, 0);
  }

  return 0;
}

int numap_sampling_write_start(struct numap_sampling_measure *measure) {

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
  pe_attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_ADDR | PERF_SAMPLE_DATA_SRC;
  pe_attr.mmap = 1;
  pe_attr.task = 1;
  pe_attr.precise_ip = 2;

  // Other parameters
  pe_attr.disabled = 1;
  pe_attr.exclude_kernel = 1;
  pe_attr.exclude_hv = 1;

  return __numap_sampling_start(measure, &pe_attr);
}

int numap_sampling_write_stop(struct numap_sampling_measure *measure) {
  return numap_sampling_read_stop(measure);
}

int numap_sampling_end(struct numap_sampling_measure *measure) {
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    munmap(measure->metadata_pages_per_tid[thread], measure->mmap_len);
    close(measure->fd_per_tid[thread]);
  }

  return 0;
}
