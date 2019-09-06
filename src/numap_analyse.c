#include <stdlib.h>
#include <stdio.h>

#include "numap.h"

int is_served_by_local_cache1(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_L1) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_local_cache2(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_L2) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_local_cache3(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_L3) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_local_lfb(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_LFB) {
      return 1;
    }
  }
  return 0;
}


int is_served_by_local_cache(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_L1) {
      return 1;
    }
    if (data_src.mem_lvl & PERF_MEM_LVL_LFB) {
      return 1;
    }
    if (data_src.mem_lvl & PERF_MEM_LVL_L2) {
      return 1;
    }
    if (data_src.mem_lvl & PERF_MEM_LVL_L3) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_local_memory(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_LOC_RAM) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_remote_cache_or_local_memory(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT && data_src.mem_lvl & PERF_MEM_LVL_REM_CCE1) {
    return 1;
  }
  return 0;
}

int is_served_by_remote_memory(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    if (data_src.mem_lvl & PERF_MEM_LVL_REM_RAM1) {
      return 1;
    } else if (data_src.mem_lvl & PERF_MEM_LVL_REM_RAM2) {
      return 1;
    }
  }
  return 0;
}

int is_served_by_local_NA_miss(union perf_mem_data_src data_src) {
  if (data_src.mem_lvl & PERF_MEM_LVL_NA) {
    return 1;
  }
 if (data_src.mem_lvl & PERF_MEM_LVL_MISS && data_src.mem_lvl & PERF_MEM_LVL_L3) {
    return 1;
  }
  return 0;
}

char *get_data_src_opcode(union perf_mem_data_src data_src) {
  char * res = concat("", "");
  char * old_res;
  if (data_src.mem_op & PERF_MEM_OP_NA) {
    old_res = res;
    res = concat(res, "NA");
    free(old_res);
  }
  if (data_src.mem_op & PERF_MEM_OP_LOAD) {
    old_res = res;
    res = concat(res, "Load");
    free(old_res);
  }
  if (data_src.mem_op & PERF_MEM_OP_STORE) {
    old_res = res;
    res = concat(res, "Store");
    free(old_res);
  }
  if (data_src.mem_op & PERF_MEM_OP_PFETCH) {
    old_res = res;
    res = concat(res, "Prefetch");
    free(old_res);
  }
  if (data_src.mem_op & PERF_MEM_OP_EXEC) {
    old_res = res;
    res = concat(res, "Exec code");
    free(old_res);
  }
  return res;
}

char *get_data_src_level(union perf_mem_data_src data_src) {
  char * res = concat("", "");
  char * old_res;
  if (data_src.mem_lvl & PERF_MEM_LVL_NA) {
    old_res = res;
    res = concat(res, "NA");
    free(old_res);
  }
  if (data_src.mem_lvl & PERF_MEM_LVL_L1) {
    old_res = res;
    res = concat(res, "L1");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_LFB) {
    old_res = res;
    res = concat(res, "LFB");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_L2) {
    old_res = res;
    res = concat(res, "L2");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_L3) {
    old_res = res;
    res = concat(res, "L3");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_LOC_RAM) {
    old_res = res;
    res = concat(res, "Local_RAM");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_REM_RAM1) {
    old_res = res;
    res = concat(res, "Remote_RAM_1_hop");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_REM_RAM2) {
    old_res = res;
    res = concat(res, "Remote_RAM_2_hops");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_REM_CCE1) {
    old_res = res;
    res = concat(res, "Remote_Cache_1_hop");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_REM_CCE2) {
    old_res = res;
    res = concat(res, "Remote_Cache_2_hops");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_IO) {
    old_res = res;
    res = concat(res, "I/O_Memory");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_UNC) {
    old_res = res;
    res = concat(res, "Uncached_Memory");
    free(old_res);
  }
  if (data_src.mem_lvl & PERF_MEM_LVL_HIT) {
    old_res = res;
    res = concat(res, "_Hit");
    free(old_res);
  } else if (data_src.mem_lvl & PERF_MEM_LVL_MISS) {
    old_res = res;
    res = concat(res, "_Miss");
    free(old_res);
  }
  return res;
}

int get_index(uint32_t tid, struct numap_sampling_measure *measure) {
  uint32_t thread;
  int index = 0;
  for (thread = 0; thread < measure->nb_threads; thread++) {
    if (measure->tids[index] == tid) {
      return index;
    }
    index++;
  }
  return -1;
}

int numap_sampling_print(struct numap_sampling_measure *measure, char print_samples) {
  int thread;
  for (thread = 0; thread < measure->nb_threads; thread++) {

    struct perf_event_mmap_page *metadata_page = measure->metadata_pages_per_tid[thread];
    uint64_t head = metadata_page -> data_head;
    rmb();
    struct perf_event_header *header = (struct perf_event_header *)((char *)metadata_page + measure->page_size);
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
    while (consumed < head) {
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
      consumed += header->size;
      header = (struct perf_event_header *)((char *)header + header -> size);
    }
    printf("\n");
    printf("head = %" PRIu64 " compared to max = %zu\n", head, measure->mmap_len);
    printf("Thread %d: %-8d samples\n", thread, total_count);
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache1_count, "local cache 1", (100.0 * cache1_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache2_count, "local cache 2", (100.0 * cache2_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, cache3_count, "local cache 3", (100.0 * cache3_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, lfb_count, "local cache LFB", (100.0 * lfb_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, memory_count, "local memory", (100.0 * memory_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, remote_cache_count, "remote cache or local memory", (100.0 * remote_cache_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, remote_memory_count, "remote memory", (100.0 * remote_memory_count / total_count));
    printf("Thread %d: %-8d %-30s %0.3f%%\n", thread, na_miss_count, "unknown l3 miss", (100.0 * na_miss_count / total_count));
    if (measure->nb_refresh > 0) {
	    measure->total_samples += (total_count % measure->nb_refresh);
    }
  }
  printf("\nTotal sample number : %d\n", measure->total_samples);

  return 0;
}

int numap_sampling_read_print(struct numap_sampling_measure *measure, char print_samples) {
  return numap_sampling_print(measure, print_samples);
}

int numap_sampling_write_print(struct numap_sampling_measure *measure, char print_samples) {
  return numap_sampling_print(measure, print_samples);
}
