## Overview

numap is a Linux library dedicated to memory profiling based on
hardware performance monitoring unit (PMU). The main objective for the
library is to provide high level abstraction for:

- Cores load requests sampling
- Cores store requests sampling

## Supported processors 

### Intel processors with family_model information (decimal / hexadecimal notations)

- Nehalem - Gainestown decline (06_26 / 06_1A)
- Nehalem - Lynfield decline (06_30 / 06_1E)
- Sandy Bridge (06_42 / 06_2A)
- Westmere - EP decline (06_44 / 06_2C)
- Ivy Bridge (06_58 / 06_3A)
- Haswell - DT decline  (06_60 / 06_3C)
- Ivy Bridge - E decline (06_62 / 06_3E)
- Haswell - E decline (06_63 / 06_3F)
- Haswell - ULT decline (06_69 / 06_45)
- Kaby Lake (06_142 / 06_8E)
- Kaby Lake - HQ decline (06_158 / 06_9E)
- Sky Lake - HQ decline (06_94 / 06_5E)

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

The goal is to fill up numap's data structures that look like this : 

``` C
struct archi your_machine_code_name = { 
	.id = /* A VALUE */ | /* A VALUE */ << 8, 
   	.name = "Your description of the machine",
	.sampling_read_event= " .... ",
	.sampling_write_event=" .... ",
	.counting_read_event=" .... ",
	.counting_write_event=" ... "
};
```

More  precisely,  we  need  to  define  the  correct  values  for  the
.sampling_read_event and .sampling_write_event fields. 

Once this is done, simply add you_machine_code_name to the list 

``` C
static struct archi *supported_archs[NB_SUPPORTED_ARCHS] = {
  ...
};
```

and increment NB_SUPPORTED_ARCHS. 


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
"model". The associated  numbers needs to be  converted to hexadecimal
and collated into the form 0xFAMILY_MODEL. 

In our case, we get 

```
06_2D
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
one can  use libpfm (v>4.0.0).  Once  libpfm is built, it  provides us
with  a binary  (libpfm/examples/shoevtinfo) that  prints the  list of
available events.

For our  example architecture, we  find that the  exact latency-fixing
parameter is  called LATENCY_ABOVE_THRESHOLD instead  of LOAD_LATENCY.
So be it! 


The numap's struct we need to define thus looks like: 

``` C
struct archi SANDY_BRIDGE_EP = { .id = 0x06 | 0x2D << 8, // 06_45
	.name   =    "Sandy Bridge micro-arch - Romley EP decline",
	.sampling_read_event= "MEM_TRANS_RETIRED:LATENCY_ABOVE_THRESHOLD:ldlat=3",
	.sampling_write_event="MEM_TRANS_RETIRED:PRECISE_STORE",
	.counting_read_event= NOT_SUPPORTED
	.counting_write_event= NOT_SUPPORTED
};
```

Note, the last  2 lines of the  struct are deprecated and  can thus be
indicated as NOT_SUPPORTED. 

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
