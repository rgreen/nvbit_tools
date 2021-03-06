/* Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Author: Oreste Villa, ovilla@nvidia.com - 2018 */

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <cassert>
#include <string>
#include <vector>

#include "tools_cuda_api_meta.h"
#define __CUDA_API_VERSION_INTERNAL
#include "cuda.h"
#include "generated_cuda_meta.h"

/* Instruction class returned by the NVBit inspection API nvbit_get_instrs */
class Instr {
  public:
    static constexpr const int RZ = 255;
    static constexpr const int PT = 7;

    enum memOpType {
        NONE,
        LOCAL,     // local memory operation
        GENERIC,   // generic memory operation
        GLOBAL,    // global memory operation
        SHARED,    // shared memory operation
        CONSTANT,  // constant memory operation
    };
    static constexpr const char* memOpTypeStr[] = {
        "NONE", "LOCAL", "GENERIC", "GLOBAL", "SHARED", "CONSTANT"};

    enum operandType {
        IMM,    // value[0] = immediate,       value[1] = unused
        REG,    // value[0] = register number, value[1] = unused
        PRED,   // value[0] = pred reg num,    value[1] = unused
        CBANK,  // value[0] = const bank id,   value[1] = const bank offset
        SREG,   // value[0] = unused,          value[1] = unused
        BREG,   // value[0] = cbu reg num,     value[1] = unused
        MREF,   // value[0] = register num Ra  value[1] = immediate
                // MREF: [Ra + imm] or [Ra | (Ra+1 << 32) + imm]
    };
    static constexpr const char* operandTypeStr[] = {
        "IMM", "REG", "PRED", "CBANK", "SREG", "BREG", "MREF"};

    typedef struct {
        /* operand type */
        operandType type;
        /* is negative */
        bool is_neg;
        /* is absolute */
        bool is_abs;
        /* value (see operandType_t enum for meaning) */
        double value[2];
    } operand_t;

    /* returns the "string"  containing the SASS, i.e. IMAD.WIDE R8, R8, R9 */
    const char* getSass();

    /* returns offset in bytes of this instruction within the function */
    uint32_t getOffset();
    /* returns the id of the instruction within the function */
    uint32_t getIdx();
    /* returns true if instruction used predicate */
    bool hasPred();
    /* returns predicate number, only valid if hasPred() == true */
    int getPredNum();
    /* returns true if predicate is negated (i.e. @!P0), only valid if hasPred()
     * == true */
    bool isPredNeg();
    /* returns full opcode of the instruction (i.e. IMAD.WIDE ) */
    const char* getOpcode();

    /* returns memOpType_t */
    memOpType getMemOpType();
    bool isLoad();
    bool isStore();
    bool isExtended();
    int getSize();

    /* get number of operands */
    int getNumOperands();
    /* get specific operand */
    const operand_t* getOperand(int num_operand);

    /* print fully decoded instruction */
    void printDecoded();
    /* prints one line instruction with idx, offset, sass */
    void print(const char* prefix = NULL);

  private:
    /* Constructor used internally by NVBit */
    Instr();
    /* Reserved variable used internally by NVBit */
    const void* reserved;
    friend class Nvbit;
    friend class Function;
};

/* basic block struct */
typedef struct { std::vector<Instr*> instrs; } basic_block_t;

/* control flow graph struct */
typedef struct {
    /* indicates the control flow graph can't be statically predetermined
     * because the function from which is belong uses jmx/brx types of branches
     * which targets depends of registers values that are known only
     * at runtime */
    bool is_degenerate;
    /* vector of basic block */
    std::vector<basic_block_t*> bbs;
} CFG_t;

/*
 * callback definitions as enumerated in tools_cuda_api_meta.h
 */
#define DEFINE_ENUM_CBID_API_CUDA(area, id, name, params) API_CUDA_##name,
typedef enum {
    API_CUDA_INVALID = 0,
    CU_TOOLS_FOR_EACH_CUDA_API_FUNC(DEFINE_ENUM_CBID_API_CUDA)
} nvbit_api_cuda_t;

extern "C" {
/*********************************************************************
 *
 *                  NVBit tool callbacks
 *     (implement these functions to get a callback from NVBit)
 *
 **********************************************************************/
/* This function is called as soon as the program starts, no GPU calls
 * should be made a this moment */
void nvbit_at_init();

/* This function is called just before the program terminates, no GPU calls
 * should be made a this moment */
void nvbit_at_term();

/* This function is called as soon as a GPU context is started and it should
 * contain any code that we would like to execute at that moment. */
void nvbit_at_ctx_init(CUcontext ctx);

/* This function is called as soon as the GPU context is terminated and it
 * should contain any code that we would like to execute at that moment. */
void nvbit_at_ctx_term(CUcontext ctx);

/* This is the function called every time a new kernel or device
 * function is loaded the first time into the GPU.
 * Inside this call-back is possible to inspect instructions or basic blocks and
 * to perform actual instrumentation */
void nvbit_at_function_first_load(CUcontext ctx, CUfunction func);

/* This is the function called every beginning (is_exit = 0) and
 * end (is_exit = 1) of a CUDA driver call.
 * cbid identifies the CUDA driver call as specified by the enum
 * nvbit_api_cuda_t, see tools_cuda_api_meta.h for the list of cbid.
 * Name is its the driver call name.
 * params is pointer to* one of the structures defined in the
 * generated_cuda_meta.h.
 * Params must be casted to the correct struct based on the cbid.
 * For instance if cbid = cuMemcpyDtoH_v2 then params must be casted to
 * (cuMemcpyDtoH_v2_params *)
 * */
void nvbit_at_cuda_event(CUcontext ctx, int is_exit,
                                    nvbit_api_cuda_t cbid,
                                    const char* event_name, void* params,
                                    CUresult* pStatus);

/*********************************************************************
 *
 *          NVBit inspection APIs  (provided by NVBit)
 *
 **********************************************************************/
/* Get vector of instructions, this function can be used only inside
 * nvbit_at_function_first_load */
const std::vector<Instr*>& nvbit_get_instrs(CUcontext ctx, CUfunction func);

/* Get control flow graph (CFG), this function can be used only inside
 * nvbit_at_function_first_load  */
const CFG_t& nvbit_get_CFG(CUcontext ctx, CUfunction func);

/* Allows to get a function name from its CUfunction */
const char* nvbit_get_func_name(CUcontext ctx, CUfunction f);

/* Get line information for a particular instruction offset if available,
 * binary must be compiled with --generate-line-info   (-lineinfo) */
bool nvbit_get_line_info(CUcontext cuctx, CUfunction cufunc, uint32_t offset,
                         char** file_name, char** dir_name, uint32_t* line);

/* Get the SM family */
uint32_t nvbit_get_sm_family(CUcontext cuctx);

/* Allows to get PC address of the function */
uint64_t nvbit_get_func_addr(CUfunction func);

/* Returns true if function is a kernel (i.e. __global__ ) */
bool nvbit_is_func_kernel(CUcontext ctx, CUfunction func);

/*********************************************************************
 *
 *          NVBit injection APIs  (provided by NVBit)
 *
 **********************************************************************/

/* Enumeration used by nvbit_insert_call to specify where we want to insert
 * the device function for a given Instr, if before or after */
typedef enum { IPOINT_BEFORE, IPOINT_AFTER } ipoint_t;

/* This function inserts a device function call named "dev_func_name",
 * before or after Instr (ipoint_t { IPOINT_BEFORE, IPOINT_AFTER}).
 * It is important to remember that calls to device functions are
 * identified by name (as opposed to function pointers) and that the
 * macro NVBIT_EXPORT_FUNC(dev_func_name) needs to be used to export
 * the device function inserted by this call. It is also important
 * to declare the device function that we want to insert as:
 *
 *        extern "C" __device__ __noinline__
 *
 * to prevent the compiler from optimizing out this device function
 * during compilation.
 *
 * Multiple device functions can be inserted before or after and the
 * order in which they get executed is defined by the order in which
 * they have been inserted. */

void nvbit_insert_call(const Instr* instr, const char * dev_func_name,
                       ipoint_t point);

/* Add argument to last injected call, value of the predicate for this
 * instruction */
void nvbit_add_call_arg_pred_val(const Instr* instr);

/* Add argument to last injected call, value of the entire predicate register
 * for this thread */
void nvbit_add_call_arg_pred_reg(const Instr* instr);

/* Add argument to last injected call, constant 32-bit value */
void nvbit_add_call_arg_const_val32(const Instr* instr, uint32_t val);

/* Add argument to last injected call, constant 64-bit value */
void nvbit_add_call_arg_const_val64(const Instr* instr, uint64_t val);

/* Add argument to last injected call, content of the register reg_num */
void nvbit_add_call_arg_reg_val(const Instr* instr, int reg_num);

/* Add argument to last injected call, 32-bit at launch value at offset
 * "offset", set at launch time with nvbit_set_at_launch */
void nvbit_add_call_arg_launch_val32(const Instr* instr, int offset);

/* Add argument to last injected call, 64-bit at launch value at offset
 * "offset", set at launch time with nvbit_set_at_launch */
void nvbit_add_call_arg_launch_val64(const Instr* instr, int offset);

/* Add argument to last injected call,  as value of constant bank at
 * c[bankid][bankoffset] */
void nvbit_add_call_arg_cbank_val(const Instr* instr, int bankid,
                                  int bankoffset);

/* Remove the original instruction */
void nvbit_remove_orig(const Instr* instr);

/*********************************************************************
 *
 *          NVBit device level APIs  (provided by NVBit)
 *
 **********************************************************************/

#ifdef __CUDACC__
/* device function used to read/write register values
 * writes are permanent into application state */
__device__ __noinline__ int32_t nvbit_read_reg(uint64_t reg_num);
__device__ __noinline__ void nvbit_write_reg(uint64_t reg_num,
                                                        int32_t reg_val);
#endif

/*********************************************************************
 *
 *          NVBit control APIs  (provided by NVBit)
 *
 **********************************************************************/

/* Run instrumented or original function based on flag value */
void nvbit_enable_instrumented(CUcontext ctx, CUfunction func, bool flag);

/* Set arguments at launch time, that will be loaded on input argument of
 * the instrumentation function */
void nvbit_set_at_launch(CUcontext ctx, CUfunction func, void* buf,
                         uint32_t nbytes);

/* Notify nvbit of a pthread used by the tool, this pthread will not
 * trigger any call backs even if executing CUDA events of kernel launches.
 * Multiple pthreads can be registered one after the other. */
void nvbit_set_tool_pthread(pthread_t tool_pthread);

/* Set nvdisasm */
void nvbit_set_nvdisasm(const char * nvdisasm);
}
/* Any device function used for instrumentation and inserted with
 * nvbit_insert_call need to be exported with this MACRO to prevent
 * "dead-code eliminate" from the compiler */
#define NVBIT_EXPORT_FUNC(f)               \
    __global__ void __nvbit_wrap__##f() {  \
        void (*pfun)() = (void (*)())f;    \
        if (pfun == (void (*)())1) pfun(); \
    }
