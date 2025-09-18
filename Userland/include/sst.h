#ifndef SST_H
#define SST_H

/*
 * Startup Self-Test entry point.
 * Runs a suite of tests to ensure correct kernel functionality. Intended to
 * run at system startup, before shell init.
 */
int sst_run_tests();

#endif
