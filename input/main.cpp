// main.cpp
//
// Created by Daniel Schwartz-Narbonne on 13/04/07.
// Modified by Christian Bienia
//
// Copyright 2007-2008 Princeton University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <fcntl.h>     /* open */
#include <unistd.h>    /* exit */
#include <sys/ioctl.h> /* ioctl */
#include <sys/mman.h>
#include <sys/time.h>
#include <numa.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#ifdef _OPENMP
#    include <omp.h>
#endif

// #include "config.h"
#define CONFIG_SHM_FILE_NAME "/tmp/mitosis-canneal-bench"

FILE *opt_file_out = NULL;  ///< standard outpu

// extern int real_main(int argc, char *argv[]);
int real_main (int argc, char * const argv[]);

void signalhandler(int sig)
{
    fprintf(opt_file_out, "<sig>Signal %i caught!</sig>\n", sig);

    FILE *fd3 = fopen(CONFIG_SHM_FILE_NAME ".done", "w");

    if (fd3 == NULL) {
        fprintf(stderr, "ERROR: could not create the shared memory file descriptor\n");
        exit(-1);
    }

    usleep(250);

    fprintf(opt_file_out, "</benchmark>\n");

    exit(0);
}


int main(int argc, char *argv[])
{
    struct timeval tstart, tend;
    gettimeofday(&tstart, NULL);

    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    /* check if NUMA is available, otherwise we don't know how to allocate memory */
    if (numa_available() == -1) {
        fprintf(stderr, "ERROR: Numa not available on this machine.\n");
        return -1;
    }

    opt_file_out = stdout;
    int c;
    while ((c = getopt(argc, argv, "o:h")) != -1) {
        switch (c) {
        case '-':
            break;
        case 'h':
            printf("usage: %s [-o FILE]\n", argv[0]);
            return 0;
        case 'o':
            opt_file_out = fopen(optarg, "a");
            if (opt_file_out == NULL) {
                fprintf(stderr, "Could not open the file '%s' switching to stdout\n", optarg);
                opt_file_out = stdout;
            }
            break;
        case '?':
            switch (optopt) {
            case 'o':
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                return -1;
            default:
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                return -1;
            }
        }
    }

    int prog_argc = 0;
    char **prog_argv = NULL;

    prog_argv = &argv[0];
    prog_argc = argc;

    optind = 1;

    for (int i = 0; i < argc; i++) {
        if (strcmp("--", argv[i]) == 0) {
            argv[i] = argv[0];
            prog_argv = &argv[i];
            prog_argc = argc - i;
            break;
        }
    }

    /* start with output */
    fprintf(opt_file_out, "<benchmark exec=\"%s\">\n", argv[0]);

    fprintf(opt_file_out, "<config>\n");
#ifdef _OPENMP
    fprintf(opt_file_out, "  <openmp>on</openmp>");
#else
    fprintf(opt_file_out, "  <openmp>off</openmp>");
#endif
    fprintf(opt_file_out, "</config>\n");

    /* setting the bind policy */
    numa_set_strict(1);
    numa_set_bind_policy(1);

    struct sigaction sigact;
    sigset_t block_set;

    sigfillset(&block_set);
    sigdelset(&block_set, SIGUSR1);

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = signalhandler;
    sigaction(SIGUSR1, &sigact, NULL);

    fprintf(opt_file_out, "<run>\n");
    real_main(prog_argc, prog_argv);
    fprintf(opt_file_out, "</run>\n");

    gettimeofday(&tend, NULL);
    printf("Total time: %zu.%03zu\n", tend.tv_sec - tstart.tv_sec,
           (tend.tv_usec - tstart.tv_usec) / 1000);

    fprintf(opt_file_out, "</benchmark>\n");
    return 0;
}

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#ifdef ENABLE_THREADS
#include <pthread.h>
#endif

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#include "annealer_types.h"
#include "annealer_thread.h"
#include "netlist.h"
#include "rng.h"

using namespace std;

void* entry_pt(void*);


int real_main (int argc, char * const argv[]);
int real_main (int argc, char * const argv[]) {
#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
        cout << "PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION) << endl << flush;
#else
        cout << "PARSEC Benchmark Suite" << endl << flush;
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_begin(__parsec_canneal);
#endif

	srandom(3);

	if(argc != 5 && argc != 6) {
		cout << "Usage: " << argv[0] << " NTHREADS NSWAPS TEMP NETLIST [NSTEPS]" << endl;
		exit(1);
	}	
	
	//argument 1 is numthreads
	int num_threads = atoi(argv[1]);
	cout << "Threadcount: " << num_threads << endl;
#ifndef ENABLE_THREADS
	if (num_threads != 1){
		cout << "NTHREADS must be 1 (serial version)" <<endl;
		exit(1);
	}
#endif
		
	//argument 2 is the num moves / temp
	int swaps_per_temp = atoi(argv[2]);
	cout << swaps_per_temp << " swaps per temperature step" << endl;

	//argument 3 is the start temp
	int start_temp =  atoi(argv[3]);
	cout << "start temperature: " << start_temp << endl;
	
	//argument 4 is the netlist filename
	string filename(argv[4]);
	cout << "netlist filename: " << filename << endl;
	
	//argument 5 (optional) is the number of temperature steps before termination
	int number_temp_steps = -1;
        if(argc == 6) {
		number_temp_steps = atoi(argv[5]);
		cout << "number of temperature steps: " << number_temp_steps << endl;
        }

	//now that we've read in the commandline, run the program
	netlist my_netlist(filename);
	
	annealer_thread a_thread(&my_netlist,num_threads,swaps_per_temp,start_temp,number_temp_steps);
	
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_begin();
#endif
#ifdef ENABLE_THREADS
	std::vector<pthread_t> threads(num_threads);
	void* thread_in = static_cast<void*>(&a_thread);
	for(int i=0; i<num_threads; i++){
		pthread_create(&threads[i], NULL, entry_pt,thread_in);
	}
	for (int i=0; i<num_threads; i++){
		pthread_join(threads[i], NULL);
	}
#else
	a_thread.Run();
#endif
#ifdef ENABLE_PARSEC_HOOKS
	__parsec_roi_end();
#endif
	
	cout << "Final routing is: " << my_netlist.total_routing_cost() << endl;

#ifdef ENABLE_PARSEC_HOOKS
	__parsec_bench_end();
#endif

	return 0;
}

void* entry_pt(void* data)
{
	annealer_thread* ptr = static_cast<annealer_thread*>(data);
	ptr->Run();
}
