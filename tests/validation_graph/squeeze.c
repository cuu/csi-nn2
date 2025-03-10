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

void op_test_run(struct csi_tensor *input, struct csi_tensor *output, struct squeeze_params *params,
                 struct csi_session *sess, struct csi_tensor *real_input, float *output_data,
                 float diff)
{
    csi_session_init(sess);
    csi_set_input_number(1, sess);
    csi_set_output_number(1, sess);
    csi_squeeze_init(input, output, params);

    csi_set_tensor_entry(input, sess);
    csi_set_input(0, input, sess);

    csi_squeeze(input, output, params);

    csi_set_output(0, output, sess);
    csi_session_setup(sess);

    csi_update_input(0, real_input, sess);
    csi_session_run(sess);
    csi_get_output(0, output, sess);

    struct csi_tensor *foutput = csi_ref_tensor_transform_f32(output);
    result_verify_f32(output_data, foutput->data, input->data, diff, csi_tensor_size(output),
                      false);

    free_input(real_input);
    csi_ref_tensor_transform_free_f32(foutput);
    csi_session_deinit(sess);
    csi_free_session(sess);
}

void test_squeeze(struct csi_tensor *input, struct csi_tensor *output,
                  struct squeeze_params *params, float difference);

int main(int argc, char** argv)
{
    init_testsuite("Testing function of squeeze(graph).\n");

    int *buffer = read_input_data_f32(argv[1]);
    int axis_len = buffer[3];
    int32_t *axis = (int32_t *)malloc(axis_len * sizeof(int32_t));
    for(int i = 0; i < axis_len; i++) {
        axis[i] = buffer[4+i];
    }

    struct csi_tensor *reference = csi_alloc_tensor(NULL);
    int in_size = 0, out_size = 0;

    /* input tensor configuration */
    struct csi_tensor *input  = csi_alloc_tensor(NULL);
    input->dim[0] = buffer[0];          // batch
    input->dim[1] = buffer[1];          // height
    input->dim[2] = buffer[2];          // width
    input->dim[3] = 1;
    input->dim[4] = 1;
    input->dim[5] = 1;
    input->dim_count = 6;
    in_size = buffer[0] * buffer[1] * buffer[2];
    input->name = "input";
    float *input_data = (float *)(buffer + 4 + axis_len);
    input->data = input_data;
    input->dtype = CSINN_DTYPE_FLOAT32;
    input->layout = CSINN_LAYOUT_NCHW;

    /* output tensor configuration */
    struct csi_tensor *output = csi_alloc_tensor(NULL);
    output->dim[0] = buffer[0];
    output->dim[1] = buffer[1];
    output->dim[2] = buffer[2];
    output->dim_count = input->dim_count - axis_len;
    out_size = buffer[0] * buffer[1] * buffer[2];
    reference->data = (float *)(buffer + 4 + axis_len + in_size);
    output->data = reference->data;
    output->name = "output";
    output->layout = CSINN_LAYOUT_NCHW;
    output->dtype = CSINN_DTYPE_FLOAT32;

    /* operator parameter configuration */
    struct squeeze_params params;
    params.base.name = "params";
    params.base.layout = CSINN_LAYOUT_NCHW;
    params.base.run_mode = CSINN_RM_NPU_GRAPH;
    params.axis = axis;
    params.axis_num = axis_len;

    /* verify result */
    float difference = argc > 2 ? atof(argv[2]) : 1e-4;
    test_squeeze(input, output, &params, difference);

    return done_testing();
}
