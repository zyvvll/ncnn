// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "shufflechannel_vulkan.h"

namespace ncnn {

DEFINE_LAYER_CREATOR(ShuffleChannel_vulkan)

ShuffleChannel_vulkan::ShuffleChannel_vulkan()
{
    support_vulkan = true;

    pipeline_shufflechannel = 0;
    pipeline_shufflechannel_pack4 = 0;
}

int ShuffleChannel_vulkan::create_pipeline(const Option& opt)
{
    std::vector<vk_specialization_type> specializations(1);
    specializations[0].i = group;

    // pack1
    {
        pipeline_shufflechannel = new Pipeline(vkdev);
        pipeline_shufflechannel->set_optimal_local_size_xyz();
        pipeline_shufflechannel->create("shufflechannel", specializations, 2, 10);
    }

    // pack4
    {
        pipeline_shufflechannel_pack4 = new Pipeline(vkdev);
        pipeline_shufflechannel_pack4->set_optimal_local_size_xyz();
        pipeline_shufflechannel_pack4->create("shufflechannel_pack4", specializations, 2, 10);
    }

    return 0;
}

int ShuffleChannel_vulkan::destroy_pipeline(const Option& opt)
{
    delete pipeline_shufflechannel;
    pipeline_shufflechannel = 0;

    delete pipeline_shufflechannel_pack4;
    pipeline_shufflechannel_pack4 = 0;

    return 0;
}

int ShuffleChannel_vulkan::forward(const VkMat& bottom_blob, VkMat& top_blob, VkCompute& cmd, const Option& opt) const
{
    int w = bottom_blob.w;
    int h = bottom_blob.h;
    int channels = bottom_blob.c;
    size_t elemsize = bottom_blob.elemsize;
    int packing = bottom_blob.packing;

    top_blob.create(w, h, channels, elemsize, packing, opt.blob_vkallocator, opt.staging_vkallocator);
    if (top_blob.empty())
        return -100;

    std::vector<VkMat> bindings(2);
    bindings[0] = bottom_blob;
    bindings[1] = top_blob;

    std::vector<vk_constant_type> constants(10);
    constants[0].i = bottom_blob.dims;
    constants[1].i = bottom_blob.w;
    constants[2].i = bottom_blob.h;
    constants[3].i = bottom_blob.c;
    constants[4].i = bottom_blob.cstep;
    constants[5].i = top_blob.dims;
    constants[6].i = top_blob.w;
    constants[7].i = top_blob.h;
    constants[8].i = top_blob.c;
    constants[9].i = top_blob.cstep;

    const Pipeline* pipeline = packing == 4 ? pipeline_shufflechannel_pack4 : pipeline_shufflechannel;

    cmd.record_pipeline(pipeline, bindings, constants, top_blob);

    return 0;
}

} // namespace ncnn
