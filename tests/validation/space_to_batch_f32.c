/*
 * Copyright (C) 2016-2021 C-SKY Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* CSI-NN2 version 1.10.x */

#include "test_utils.h"
#include "csi_nn.h"
#include "math_snr.h"

int main(int argc, char** argv)
{
    init_testsuite("Testing function of space_to_depth f32.\n");

    struct csi_tensor *input = csi_alloc_tensor(NULL);
    struct csi_tensor *output = csi_alloc_tensor(NULL);
    struct csi_tensor *reference = csi_alloc_tensor(NULL);
    struct space_to_batch_params params;
    int in_size = 0;
    int out_size = 0;

    int *buffer = read_input_data_f32(argv[1]);

    input->dim[0] = buffer[0];   //batch
    input->dim[1] = buffer[1];   //in_channel
    input->dim[2] = buffer[2];   //in_height
    input->dim[3] = buffer[3];   //in_width

    params.block_size = buffer[4];
    params.pad_top = buffer[5];
    params.pad_bottom = buffer[6];
    params.pad_left = buffer[7];
    params.pad_right = buffer[8];

    output->dim[0] = input->dim[0] * params.block_size * params.block_size;
    output->dim[1] = input->dim[1];
    output->dim[2] = (input->dim[2] + params.pad_top + params.pad_bottom) / params.block_size;
    output->dim[3] = (input->dim[3] + params.pad_left + params.pad_right) / params.block_size;

    input->dim_count = 4;
    output->dim_count = 4;
    input->dtype = CSINN_DTYPE_FLOAT32;
    output->dtype = CSINN_DTYPE_FLOAT32;

    in_size = input->dim[0] * input->dim[1] * input->dim[2] * input->dim[3];
    out_size = output->dim[0] * output->dim[1] * output->dim[2] * output->dim[3];
    params.base.api = CSINN_API;
    params.base.run_mode = CSINN_RM_LAYER;

    input->data = (float *)(buffer + 9);
    reference->data = (float *)(buffer + 9 + in_size);
    output->data = malloc(out_size * sizeof(float));
    float difference = argc > 2 ? atof(argv[2]) : 0.9;

    if (csi_space_to_batch_init(input, output, &params) == CSINN_TRUE) {
        csi_space_to_batch(input, output, &params);
    }

    result_verify_f32(reference->data, output->data, input->data, difference, out_size, false);

    free(buffer);
    free(output->data);
    return done_testing();
}