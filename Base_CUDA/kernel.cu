
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <stdio.h>
#include <iostream>

// Simplified NVidia CUDA 11.7 Visual Studio Sample

void callCudaKernelWrapper(const int offset, const int *input, int* output, unsigned int size);

__global__ void offsetCounterKernel(const int offset, const int* input, int* output)
{
    int i = threadIdx.x;
    output[i] = input[i] + offset + i;
}

#define CheckCudaError(fn) if(fn != cudaError::cudaSuccess){ std::cout << #fn << "\n" << __FILE__ << ":" << __LINE__ << "  <-- FAILED!\n"; exit(0);}

int main()
{
    const   int arraySize         = 5;
    const   int offset            = 10;
    const   int input[arraySize]  = { 0, 0, 0, 0, 0 };
            int output[arraySize] = { 0 };

    // Add vectors in parallel.
    callCudaKernelWrapper(offset, input, output, arraySize);

    printf("{%d,%d,%d,%d,%d} + {%d} + {thread_index} = {%d,%d,%d,%d,%d}\n",
            input[0], input[1], input[2], input[3], input[4],
            offset,
            output[0], output[1], output[2], output[3], output[4]);

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    CheckCudaError(cudaDeviceReset());

    return 0;
}

void callCudaKernelWrapper(const int offset, const int* input, int* output, unsigned int size)
{
    int * cuda_input = 0;
    int * cuda_output = 0;

    // Choose which GPU to run on, change this on a multi-GPU system.
    CheckCudaError(cudaSetDevice(0));

    // Allocate GPU buffers for three vectors (two input, one output)    .
    CheckCudaError(cudaMalloc((void**)&cuda_input, size * sizeof(int)));
    CheckCudaError(cudaMalloc((void**)&cuda_output, size * sizeof(int)));

    // Copy input vectors from host memory to GPU buffers.
    CheckCudaError(cudaMemcpy(cuda_input, input, size * sizeof(int), cudaMemcpyHostToDevice));

    // Launch a kernel on the GPU with one thread for each element.
    offsetCounterKernel <<<1, size>>>(offset, cuda_input, cuda_output);

    // Check for any errors launching the kernel
    CheckCudaError(cudaGetLastError());
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    CheckCudaError(cudaDeviceSynchronize());

    // Copy output vector from GPU buffer to host memory.
    CheckCudaError(cudaMemcpy(output, cuda_output, size * sizeof(int), cudaMemcpyDeviceToHost));
}
