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
    init_testsuite("Testing function of stack(anole).\n");

    int *buffer = read_input_data_f32(argv[1]);
    int input_cnt = buffer[0];
    int axis = buffer[1];
    int output_dims = buffer[2];

    struct csi_tensor *reference = csi_alloc_tensor(NULL);
    int in_size = 0, out_size = 1;

    /* session configuration */
    struct csi_session *sess = csi_alloc_session();
    sess->base_api = CSINN_ANOLE;
    csi_session_init(sess);
    csi_set_input_number(input_cnt, sess);
    csi_set_output_number(1, sess);


    /* input tensor configuration */
    struct csi_tensor *input[input_cnt];
    float *src_in[input_cnt];
    uint8_t **src_tmp = (uint8_t **)malloc(input_cnt * sizeof(uint8_t *));
    char input_name[input_cnt][10];
    for(int i = 0; i < input_cnt; i++) {
        input[i]  = csi_alloc_tensor(sess);
        input[i]->dim_count = output_dims - 1;
        in_size = 1;
        for (int j = 0; j < input[i]->dim_count; j++) {
            if (j < axis) {
                input[i]->dim[j] = buffer[3+j];     // input[i]->dim[j] = output->dim[j]
            } else {
                input[i]->dim[j] = buffer[3+j+1];   // input[i]->dim[j] = output->dim[j + 1]
            }
            in_size *= input[i]->dim[j];
        }
        src_in[i] = (float *)(buffer + 3 + output_dims + in_size * i);
        input[i]->data = src_in[i];
        get_quant_info(input[i]);

        src_tmp[i] = (uint8_t *)malloc(sizeof(uint8_t) * in_size);
        for(int j = 0; j < in_size; j++) {
            src_tmp[i][j]=csi_ref_quantize_f32_to_u8(src_in[i][j], input[i]->qinfo);
        }
        sprintf(input_name[i], "input_%d", i);
        input[i]->name = input_name[i];
    }


    /* output tensor configuration */
    struct csi_tensor *output = csi_alloc_tensor(sess);
    output->dim_count = output_dims;
    for(int i = 0; i < output->dim_count; i++) {
        output->dim[i] = buffer[3 + i];
        out_size *= output->dim[i];
    }
    reference->data = (float *)(buffer + 3 + output->dim_count + in_size * input_cnt);
    output->data = reference->data;
    output->name = "output";
    get_quant_info(output);


    /* operator parameter configuration */
    struct stack_params params;
    params.base.api = CSINN_API;
    params.base.layout = CSINN_LAYOUT_NCHW;
    params.base.run_mode = CSINN_RM_NPU_GRAPH;
    params.base.name = "params";
    params.inputs_count = input_cnt;
    params.axis = axis;


    if (csi_stack_init((struct csi_tensor **)&input, output, &params) != CSINN_TRUE) {
        printf("stack init fail.\n\t");
        return -1;
    }

    for(int i = 0; i < input_cnt; i++) {
        csi_set_tensor_entry(input[i], sess);
        csi_set_input(i, input[i], sess);
    }

    csi_stack((struct csi_tensor **)&input, output, &params);

    csi_set_output(0, output, sess);
    csi_session_setup(sess);


    struct csi_tensor *input_tensor[input_cnt];
    for (int i = 0; i < input_cnt; i++) {
        input_tensor[i] = csi_alloc_tensor(NULL);
        input_tensor[i]->data = src_tmp[i];
        csi_update_input(i, input_tensor[i], sess);
    }
    csi_session_run(sess);

    struct csi_tensor *output_tensor = csi_alloc_tensor(NULL);
    output_tensor->data = NULL;
    output_tensor->dtype = sess->base_dtype;
    output_tensor->is_const = 0;
    int output_num = csi_get_output_number(sess);
    printf("output_num = %d\n", output_num);
    csi_get_output(0, output_tensor, sess);
    memcpy(output_tensor->qinfo, output->qinfo, sizeof(struct csi_quant_info));

    /* verify result */
    float difference = argc > 2 ? atof(argv[2]) : 1e-4;
    result_verify_8(reference->data, output_tensor, input[0]->data, difference, out_size, false);

    /* free alloced memory */
    free(buffer);
    for (int i = 0; i < input_cnt; i++) {
        free(input_tensor[i]->qinfo);
        free(input_tensor[i]);
    }
    free(output_tensor->qinfo);
    free(output_tensor);
    free(reference->qinfo);
    free(reference);
    free(src_tmp);

    csi_session_deinit(sess);
    csi_free_session(sess);
    return done_testing();
}
