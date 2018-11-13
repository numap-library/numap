## Overview

numap is a Linux library dedicated to memory profiling based on
hardware performance monitoring unit (PMU). The main objective for the
library is to provide high level abstraction for:

- Cores load requests sampling
- Cores store requests sampling

## Supported processors 

### Intel processors with family_model information (decimal notation)

- Nehalem (06_26, 06_30, 06_31, 06_46)
- Sandy Bridge (06_42, 06_45)
- Westmere (06_37, 06_44, 06_44)
- Ivy Bridge (06_58, 06_62)
- Haswell  (06_60, 06_63, 06_69, 06_70)
- Broadwell (06_61, 06_71, 06_79, 06_86)
- Kaby Lake (06_142, 06_158)
- Sky Lake (06_94, 06_78)
- Cannon Lake (06-102)
- Ice Lake (06_126)

### Not implemented Intel processors:

- Knights Ferry (11_00)
- Knights Corner (11_01)
- Knights Mill (06_133)
- Knights Landing (06_87)

### AMD processors

 - On going development

## Folders Organization

- `examples`: contains some examples showing how to use numap.

- `include`: contains numap headers

- `src`: contains numap implementation files

- `Makefile`: is a Makefile building both the library and the examples

## Dependencies

- libpfm4
- libnuma

## Howto: extend numap in ordre to take your processor model into account. 


### Intro

The goal is to tell numap which read/write events to use on a specific architecture. The `get_archi` function specifies for each architecture which events to use:

``` C
switch(archi_id) {
/* ... */
  case CPU_MODEL(6, 158):
  case CPU_MODEL(6, 142):
    snprintf(arch->name, 256, "Kaby Lake micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LOAD_LATENCY:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_INST_RETIRED:ALL_STORES");
    break;
```

You can add a new architecture by adding a new case.

### Getting the correct info

On the machine considered, type 

```
less /proc/cpuinfo
```

This file contains info in the following form: 

```
processor       : 0
vendor_id       : GenuineIntel
cpu family      : 6
model           : 45
model name      : Intel(R) Xeon(R) CPU E5-2630 0 @ 2.30GHz
stepping        : 7
microcode       : 0x710
cpu MHz         : 1339.121
cache size      : 15360 KB
physical id     : 0
siblings        : 12
core id         : 0
cpu cores       : 6
apicid          : 0
initial apicid  : 0
fpu             : yes
fpu_exception   : yes
cpuid level     : 13
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc aperfmperf eagerfpu pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic popcnt tsc_deadline_timer aes xsave avx lahf_lm ida arat epb xsaveopt pln pts dtherm tpr_shadow vnmi flexpriority ept vpid
bogomips        : 4599.76
clflush size    : 64
cache_alignment : 64
address sizes   : 46 bits physical, 48 bits virtual
power management:
```

Amongst this  info, you are interested  in the lines "cpu  family" and
"model". Using them, you can add a new case:

``` C
case CPU_MODEL(cpu_family, model):
```

In our case, we get 

``` C
case CPU_MODEL(06, 45):
```

In  the Intel  documentations, this  will be  noted as  06_2DH (H  for
... hexa)

Now, open  the Intel  documentation called  "64, IA,  32 Architectures
Software Developer Manual", and search for the string FAMILY_MODEL (in
our example  06_2D). This brings you,  among others into a  section of
chapter 19. Chapter 19 is called Performance Monitoring Events. In our
case, we  find that  06_2DH is described  in section  19.6 PERFORMANCE
MONITORING  EVENTS FOR  2ND GENERATION  INTEL® CORETM  I7-2XXX, INTEL®
CORETM I5-2XXX, INTEL® CORETM I3-2XXX PROCESSOR SERIES 


In the table provided in this section, find the lines corresponding to
the requried  info. In  particular, in  this example,  we fill  in the
values for sampling_read_event and  sampling_write_event. We leave out
thos for counting_read_event and counting_write_event

#### .sampling_read_event

For the sampling of memory reads, you need something like: 

```
| CDH | 01H | MEM_TRANS_RETIRED.LOAD_LATENCY  | Randomly sampled loads whose latency is above a user defined threshold. A small fraction of the overall loads are sampled due to randomization. PMC3 only. | Specify threshold in MSR 3F6H. |
```


#### .sampling_write_event

```
| CDH | 02H | MEM_TRANS_RETIRED.PRECISE_STORE  | Sample stores and collect precise store operation via PEBS record. PMC3 only. | See Section 18.9.4.3. |
```

### Filling up numap's struct archi for your machine

On some architectures, the info  provided in the general documentation
is INCORRECT.  To  get the correct naming  of the sampling_read_event,
one can  use the `examples/showevtinfo` program provided by numap. This
program prints the  list of available events.

For our  example architecture, we  find that the  exact latency-fixing
parameter is  called LATENCY_ABOVE_THRESHOLD instead  of LOAD_LATENCY.
So be it! 


Thus, we modify get_archi to add these lines:

``` C
  case CPU_MODEL(6, 45):
    snprintf(arch->name, 256, "Sandy Bridge micro arch");
    snprintf(arch->sampling_read_event, 256, "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3");
    snprintf(arch->sampling_write_event, 256, "MEM_TRANS_RETIRED:PRECISE_STORE");
    break;
```

### Testing

When this is done go to numap's root directory, type 

``` 
$ cmake
$ make
```

Then try the example binary in examples: 

```
$ examples/example
```

This program should output something looking like: 

```
root@taurus-8 ~/numap:-)examples/example

Starting memory read sampling
Memory read sampling results

head = 192200 compared to max = 266240
Thread 0: 4805     samples
Thread 0: 4805     local cache 1                  100.000%
Thread 0: 0        local cache 2                  0.000%
Thread 0: 0        local cache 3                  0.000%
Thread 0: 0        local cache LFB                0.000%
Thread 0: 0        local memory                   0.000%
Thread 0: 0        remote cache or local memory   0.000%
Thread 0: 0        remote memory                  0.000%
Thread 0: 0        unknown l3 miss                0.000%

head = 193240 compared to max = 266240
Thread 1: 4831     samples
Thread 1: 4831     local cache 1                  100.000%
Thread 1: 0        local cache 2                  0.000%
Thread 1: 0        local cache 3                  0.000%
Thread 1: 0        local cache LFB                0.000%
Thread 1: 0        local memory                   0.000%
Thread 1: 0        remote cache or local memory   0.000%
Thread 1: 0        remote memory                  0.000%
Thread 1: 0        unknown l3 miss                0.000%

Starting memory write sampling
Memory write sampling results

head = 262112 compared to max = 266240
Thread 0: 6452     samples
Thread 0: 6442     local cache 1                  99.845%
Thread 0: 0        local cache 2                  0.000%
Thread 0: 0        local cache 3                  0.000%
Thread 0: 0        local cache LFB                0.000%
Thread 0: 0        local memory                   0.000%
Thread 0: 0        remote cache or local memory   0.000%
Thread 0: 0        remote memory                  0.000%
Thread 0: 0        unknown l3 miss                0.000%

head = 262136 compared to max = 266240
Thread 1: 6451     samples
Thread 1: 6436     local cache 1                  99.767%
Thread 1: 0        local cache 2                  0.000%
Thread 1: 0        local cache 3                  0.000%
Thread 1: 0        local cache LFB                0.000%
Thread 1: 0        local memory                   0.000%
Thread 1: 0        remote cache or local memory   0.000%
Thread 1: 0        remote memory                  0.000%
Thread 1: 0        unknown l3 miss                0.000%
```

Congrats, numap is set up for your machine!

Don't forget to push your modifications to github of course :)
