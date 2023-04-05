# TeaLeaf HIP

A HIP port of the TeaLeaf mini-app starting from the current CUDA version.

# Extra tea.in flags

Turn on cuda/HIP kernel use by putting `use_cuda_kernels` in tea.in.

## Compiling

In most cases one needs to load the appropriate modules (including those for the accelerator in question).

Then for AMD GPUs (with the Cray compiler)

```
make clean; make COMPILER=CRAY
```
