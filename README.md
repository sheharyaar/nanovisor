# NanoVisor

A minimal implementation of a Type 2 Hypervisor for x86-64 bit Linux Host OS.

## TODO

For a single VM with simple instructions
- Write a sample AMD SVM program
- Multiple hypervisor clashes (EFER.SVME)
- VMRUN and #VMEXIT with VMCB for a single instruction
- ioctl interface to interact with the hypervisor and send programs to run

Later move to memory and I/O and then to multiple VMs