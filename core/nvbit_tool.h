/* This file needs to be include once in your nvbit tool, it provides hooks to
 * the nvbit core library to properly load this tool. Do not modify!!!  */
#pragma once
#include <stdio.h>
#include <cassert>
#include <stdint.h>

__managed__ volatile int32_t __nvbit_var = 0;

/* DO NOT change the content of these functions as this is not the real
 * implementation (reg num is 64-bit in order to make sure R4 and R5 are
 * both saved before jumping in this function) */
extern "C" __device__ __noinline__ int32_t nvbit_read_reg(uint64_t reg_num) {
#pragma unroll
    for (int i = 0; i < 1024; i++) __nvbit_var += i;
    assert(__nvbit_var == reg_num);
    return __nvbit_var;
}

extern "C" __device__ __noinline__ void nvbit_write_reg(uint64_t reg_num,
                                                        int32_t reg_val) {
#pragma unroll
    for (int i = 0; i < 1024; i++) __nvbit_var += i;
    assert(__nvbit_var == reg_num + reg_val);
}

__global__ void load_module_nvbit_kernel(int var) {
    printf("");
    if (var) {
        int tmp = nvbit_read_reg(var);
        nvbit_write_reg(var, tmp);
    }
}
extern "C" void __nvbit_start();

extern "C" void nvbit_at_context_init_hook() {
    __nvbit_start();
    load_module_nvbit_kernel<<<1, 1>>>(0);
    cudaDeviceSynchronize();
    assert(cudaGetLastError() == cudaSuccess);
}
