#include "numap.h"
#include <stdio.h>
#include <unistd.h>

int main() {

  // Init numap
  int res = numap_init();
  if(res < 0) {
    fprintf(stderr, "numap_init : %s\n", numap_error_message(res));
    return -1;
  }

  // Init measure
  struct numap_counting_measure m;
  res = numap_counting_init_measure(&m);
  if(res < 0) {
    fprintf(stderr, "numap_init_measure error : %s\n", numap_error_message(res));
    return -1;
  }

  //printf("Memory Bandwidth Monitoring\n\n");
  printf("%-5s%-20s%-20s\n", "node", "read (MiB\\s)", "write (MiB\\s)");

  while (1) {

    // Start memory controler read and writes counting
    res = numap_counting_start(&m);
    if(res < 0) {
      fprintf(stderr, "numap_start error : %s\n", numap_error_message(res));
      return -1;
    }

    sleep(1);

    // Stop memory controler read and writes counting
    res = numap_counting_stop(&m);
    if(res < 0) {
      printf("numap_stop error : %s\n", numap_error_message(res));
      return -1;
    }

    // Print memory counting results
    for(int i = 0; i < m.nb_nodes; i++) {
      //printf("%-5d%-20lld%-20lld\n", i, m.reads_count[i], m.writes_count[i]);
      long r = (m.reads_count[i] * 64) / 1024 / 1024;
      long w = (m.writes_count[i] * 64) / 1024 / 1024;
      printf("%-5d%-20" PRIu64 "%-20" PRIu64 "\n", i, r, w);
    }
    fflush(stdout);
    for(int i = 0; i < m.nb_nodes; i++) {
      printf("\033[A\033[2K");
    }
  }
  return 0;
}
