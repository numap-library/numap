## Overview

numap is a Linux library dedicated to memory profiling based on
hardware performance monitoring unit (PMU). The main objective for the
library is to provide high level abstraction for:

- Memory bandwidth profiling
- Cores load requests sampling
- Cores store requests sampling

The memory bandwidth abstraction is based on hardware counters at
  memory controller levels whereas load and store sampling are based
  on modern hardware profiling mechanisms.

## Supported processors

- Intel Xeon_E_7450 (family_model 06_29)
- Intel I7_870 (06_30)
- Intel WESTMERE_EP (O6_44)
- Intel Xeon_E5_2670 (06_45)
- Intel I5_2520 (06_42)
- I7_3770 (06_58)
- I7_5960X (06_63)

## Folders Organization

- *examples*: contains some examples showing how to use numap. One of
  these examples is a memory bandwidth reporting live tool.

- *include*: contains the numap headers

- *src*: contains numap implementation files

- *Makefile*: is a simple Makefile building both the library and the examples

## Dependencies

- libpfm4