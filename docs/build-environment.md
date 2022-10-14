# Consideration of build machine and development environment

## How to select the build machine

The development cycle of any software project has a loop of (*) build the source and running the software to find the bug, (*) write or revise the source to fix the bug.

Having as short as possible of the time for conducting the above loop will increase development productivity dramatically. Shorter loop time will be able to achieve more iteration in an hour. High number of iterations per time is the key to productivity.

In projects such as the TEEP-Device development, it is essential to have a fast machine as much as possible for building the sources and using multiple terminals for efficient development.
Selection of a fast computer is mandatory because the speed of the build machine affects the efficiency of the development.

Oftenly developers using a laptop, are logging in to a fast remote computer and building the sources on the remote machine rather than on a local laptop.

The three key components of the fast-build machine are speed of CPU, speed of storage, and memory size. The laptop will be slower than the server or desktop machines because of the limited capability of the three key components.

The frequency of the CPU having above 3.8Ghz is ideal. The write speed of the storage has a significant impact on the build time. It is almost a must to use SSD than HDD and the SSD should have above 3000MB/s write speed which is only available with M.2 form factor with NVMe interface. The 32GB or higher memory size is recommended, since it will reduce disk swapping occurring when running out of memory, which significantly increases the build time. Please request reasonable development machines if you are working at a corporate or an organization.

Some Examples of high-end machines: HPE ProLiant DL325 Gen10 with AMD EPYC 7302P 16-Core Processor, PowerEdge R7515 with AMD EPYC 7402P 24-Core Processor

## How to setup an efficient development environment

To efficiently develop TEEP-Device source code, it is good to have three terminals.

- Terminal 1: To Run tamproto
- Terminal 2: To build and run the TEEP-Device.
- Terminal 3: To modify the TEEP-Device source code.

All three terminals opened simultaneously for efficiency. Logging in to the fast build machines mentioned in the previous chapter with the command `ssh -X user@Ip_address_build_machine` will provide forwarding X-Windows on your local machine.

@image html docs/images/3-terminals.png
@image latex docs/images/3-terminals.png width=\textwidth

In the above image, you can see that terminal1 is running tamproto, terminal2 is used for building and running TEEP-Device to catch the build errors and runtime errors. The terminal3 is used for editing the source code for debugging the errors found on terminal 2 and terminal 1. At the terminal 3, use your favorite editor or IDE such as Visual Studio Code.

Every time you update the source code in terminal 3, you can rebuild the source code on terminal2 and run it and see the debug messages of TEEP-Device at terminal 2 and logs message of tamproto at terminal 1 to find the eros whiteout distracted from other terminals.

The 27 inch or larger external monitor would suit best to operate multiple terminals during developments in this usage. The selection of the resolution is also important, not too dense and not too sparse.  The 2560 x 1440 is often ideal on a 27 inch monitor, and 4K 3840 x 2160 on a 32 inch. Having more text on one display increases readability of the source code, as long as the size of the character is not too small.

