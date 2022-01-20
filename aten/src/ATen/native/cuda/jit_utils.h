#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <c10/util/irange.h>
#include <ATen/cuda/detail/LazyNVRTC.h>
#include <ATen/cuda/nvrtc_stub/ATenNVRTC.h>

#ifdef __HIP_PLATFORM_HCC__
    // pass -- jiterator not supported on HIP platforms
    #define jiterator_stringify(...) std::string("USE_JITERATOR is undefined");
#else
    #define jiterator_stringify(...) std::string(#__VA_ARGS__);
#endif

namespace at { namespace cuda { namespace jit {

enum class BinaryFuncVariant {NoScalar, RhsScalar, LhsScalar};

struct NvrtcFunction {
  CUmodule module = CUmodule();
  CUfunction function = nullptr;
};

std::string generate_code(
    int nTensors,
    const std::string& func,
    const std::string& name,
    const std::string& f_input_type,
    const std::string& compute_type,
    const std::string& result_type,
    bool contiguous,
    bool dynamic_casting,
    BinaryFuncVariant scalar_pos,
    bool vectorized=false,
    int vec_size=0);

NvrtcFunction jit_pwise_function(
    const std::string& code,
    const std::string& kernel_name);

void launch_jitted_pwise_function(
    NvrtcFunction function,
    std::array<void*, 7>& args,
    const int nBlocks,
    const int kBlockSize);


// Defines type names
template <typename T> inline std::string typeName() {
    TORCH_INTERNAL_ASSERT(false, "invalid type");
    return "void";
}

#define TYPE_NAME_FN(ctype, name) \
template <> inline std::string typeName<ctype>(){ \
    return std::string(#ctype);    \
}

AT_FORALL_SCALAR_TYPES(TYPE_NAME_FN)
#undef TYPE_NAME_FN
// JIT uses std::complex directly, because nvRTC compile programs
// with -default-device there was such issue like:
//   "std::sin(complex) is __host__ only"
template <> inline std::string typeName<c10::complex<float>>(){
    return "std::complex<float>";
}
template <> inline std::string typeName<c10::complex<double>>(){
    return "std::complex<double>";
}
template <> inline std::string typeName<at::Half>(){
    return "at::Half";
}
template <> inline std::string typeName<at::BFloat16>(){
    return "at::BFloat16";
}

}}}  // namespace at::cuda::jit
