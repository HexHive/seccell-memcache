#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tlbcache.h"

#define LINEBUFLEN        4096
#define MAXTOKENS         16
#define WARMUP_REPS       2

uint64_t lines_handled = 0;
void status_handler(int x, siginfo_t *c, void *v) {
  // fprintf(stderr, "Lines handled %"PRIu64"\n", lines_handled);
}

void timer_setup() {
  struct sigevent sev;
  timer_t timerid;
  struct sigaction sa;
  struct itimerspec its;

  /****** Set up timer to periodically print stats *******/
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIGRTMIN;
  sev.sigev_value.sival_ptr = &timerid;
  if(timer_create(CLOCK_REALTIME, &sev, &timerid)) {
    printf("ERROR: timer_create");
    exit(-1);
  }

  its.it_value.tv_sec = 2;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;
  if(timer_settime(timerid, 0, &its, NULL) == -1) {
    printf("ERROR: timer_settime");
    exit(-1);
  }

  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = status_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
    printf("ERROR: sigaction");
    exit(-1);
  }
  /******************************************************/
}

int main(int argc, char **argv) {
  FILE *tracef;
  char linebuf[LINEBUFLEN];
  char *tokens[MAXTOKENS], *token;
  int ntoken;
  uint64_t inst_addr, data_addr, data_size;
  access_t acc;
  int reps;
  bool repstarted;

  if(argc != 2) {
    printf("Usage: %s <tracefile>\n", argv[0]);
    exit(1);
  }

  timer_setup();

  init_tlbcache();
  instrumentation_enable = true;
  warmup = true;
  repstarted = false;

  tracef = fopen(argv[1], "r");
  while(fscanf(tracef, "%[^\n]\n", linebuf) != EOF) {
    // printf("%s\n", linebuf);

    token = strtok(linebuf, " ");
    ntoken = 0;
    while(token != NULL) {
      tokens[ntoken++] = token;
      token = strtok(NULL, " ");
    }

    if(strcmp(tokens[2], "start") == 0) {
      assert(!repstarted);
      repstarted = !repstarted;
      // printf("Started rep %d\n", reps);
    } else if(strcmp(tokens[2], "end") == 0) {
      assert(repstarted);
      repstarted = !repstarted;
      // printf("Ended rep %d\n", reps);
      /* End warmup after one repetition */
      if(++reps == WARMUP_REPS)
        warmup = false;
    } else if (repstarted) {
      inst_addr = strtoul(tokens[0], NULL, 16);
      data_addr = strtoul(tokens[2], NULL, 16);
      data_size = strtoul(&tokens[3][1], NULL, 10);

      // printf("%lu\n", data_size);
      acc.addr = data_addr;
      acc.size = data_size;
      cycle_access(acc, 0, DATA_ACCESS);
      account_data();
      account_inst();
    }

    lines_handled++;
    // exit(1);
  }

  printf("Rounds total: %d, warmup: %d\n", reps, WARMUP_REPS);
  account_final();

  return 1;
}