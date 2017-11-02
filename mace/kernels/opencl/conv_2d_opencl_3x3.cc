//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include "mace/core/common.h"
#include "mace/core/runtime/opencl/opencl_runtime.h"
#include "mace/kernels/conv_2d.h"

namespace mace {
namespace kernels {

static void InnerConv2dK3x3S12(const Tensor *input, const Tensor *filter,
                               const Tensor *bias, const uint32_t stride, Tensor *output) {
  const index_t channels = output->shape()[1];
  const index_t height = output->shape()[2];
  const index_t width = output->shape()[3];

  MACE_CHECK(input->dim(0) == output->dim(0));

  const index_t channel_blocks = (channels + 3) / 4;
  const index_t pixel_blocks = (width + 3) / 4 * height;

  auto runtime = OpenCLRuntime::Get();
  auto program = runtime->program();
  auto bm_kernel = cl::Kernel(program, "conv_2d_3x3");

  uint32_t idx = 0;
  bm_kernel.setArg(idx++, *(static_cast<const cl::Buffer *>(input->buffer())));
  bm_kernel.setArg(idx++, *(static_cast<const cl::Buffer *>(filter->buffer())));
  bm_kernel.setArg(idx++, *(static_cast<const cl::Buffer *>(bias->buffer())));
  bm_kernel.setArg(idx++, *(static_cast<cl::Buffer *>(output->buffer())));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(input->dim(1)));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(channels));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(input->dim(2)));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(input->dim(3)));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(height));
  bm_kernel.setArg(idx++, static_cast<uint32_t>(width));
  bm_kernel.setArg(idx++, stride);
  bm_kernel.setArg(idx++, stride);
  const uint32_t gws[3] = {static_cast<uint32_t>(output->dim(0)),
                           static_cast<uint32_t>(channel_blocks),
                           static_cast<uint32_t>(pixel_blocks)};
  const uint32_t lws[3] = {static_cast<uint32_t>(1),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(256)};
  cl_int error = runtime->command_queue().enqueueNDRangeKernel(
      bm_kernel, cl::NullRange,
      cl::NDRange(gws[0], gws[1], gws[2]),
      cl::NDRange(lws[0], lws[1], lws[2]));
  MACE_CHECK(error == CL_SUCCESS);
}

void Conv2dOpenclK3x3S1(const Tensor *input, const Tensor *filter,
                        const Tensor *bias, Tensor *output) {
  InnerConv2dK3x3S12(input, filter, bias, 1, output);
};

void Conv2dOpenclK3x3S2(const Tensor *input, const Tensor *filter,
                        const Tensor *bias, Tensor *output) {
  InnerConv2dK3x3S12(input, filter, bias, 2, output);
};

}  // namespace kernels
}  // namespace mace