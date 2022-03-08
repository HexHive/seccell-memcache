# Experiment: memcached-like microbenchmark

We create a (single-threaded) in-memory key-value store.

> Note: We shamelessly copy code from `memcached`.

## Aim

The aim of this experiment is to demonstrate:

- Comparison between range-based translation and page-based translation for a 'realistic' application.
  The application is a caching service loosely based on `memcached`.
  The range-based translation is meant to enable better frontside-TLB/range-TLB hit rates.
- *Yet not done* Compartmentalization benefit of SDSwitch.

## Implementation *as planned*

`src` implements a `memcached`-like library for caching (first fixed-size requests, then variable).

`memtrace` generates a memory access trace of the application. 
We have two options, to use QEMU to get a full-system trace, or PIN to get a user-only trace.

`tlbcache` simulates the memory access pattern to generate translation and data access latencies.

### TODO:

- Implement a 
