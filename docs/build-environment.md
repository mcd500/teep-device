# Consideration of build machine and development environment

## How to select the build machine

It is essential to have a fast machine as much as possible for building the sources and using multiple terminals for efficient development.
Selection of a fast computer is mandatory because the speed of the build machine affects the efficiency of the development.

More Often developers using a laptop, are logging in to a fast remote computer and building the sources on the remote machine rather then on a local laptop.

The three key components of the fast-build machine are CPU, storage, and memory size. T
The laptop will be slower than the server or desktop machine because of the limited capability of the three key components.

The frequency of the CPU is above 3.8Ghz is ideal. The write speed of the storage has a significant impact on the build time. It is almost a must to use SSD than HDD and the SSD should be above 3000MB/s write speed which is only available with M.2 form factor with NVMe interface. The 32GB or higher memory size is recommended, since it will prevent disk swapping when running out of memory, significantly reducing the build speed. Please request reasonable development machines if you are working at a corporate or organization.

Some Examples of high-end machines: HPE ProLiant DL325 Gen10 with AMD EPYC 7302P 16-Core Processor, PowerEdge R7515 with AMD EPYC 7402P 24-Core Processor

## How to setup an efficient development environment

To efficiently develop TEEP-Device source code, it is good to have three terminals.

Terminal 1: To Run tamproto
Terminal 2: To build the TEEP-Device source code.
Terminal 3: To modify the TEEP-Device source code.

All three terminals opened simultaneously for efficiency.

@image html docs/images/3-terminals.png
@image latex docs/images/3-terminals.png width=\textwidth

In the above image, you can see that terminal1 is running tamproto, terminal2 is used for building the modified source code and terminal3 is used for modifying the source code.
Every time the source code is updated, you can rebuild the source code on terminal2.

A 27 inch external monitor would suit best to operate multiple windows at once.

