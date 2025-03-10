/*
 * Copyright (C) 2016-2019 C-SKY Limited. All rights reserved.
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

/* ----------------------------------------------------------------------
 * Title:        csi_nn_activations_q15.c
 * Description:  Q15 neural network activation function using direct table look-up
 *
 * -------------------------------------------------------------------- */

#include "csi_math.h"
#include "csi_nn_tables.h"
// #include "csi_common_tables.h"
#include "csi_nnfunctions.h"

/**
 *  @ingroup groupNN
 */

/**
 * @addtogroup Acti
 * @{
 */

  /**
   * @brief Q15 neural network activation function using direct table look-up
   * @param[in,out]   data        pointer to input
   * @param[in]       size        number of elements
   * @param[in]       int_width   bit-width of the integer part, assume to be smaller than 3
   * @param[in]       type        type of activation functions
   * @return none.
   *
   * @details
   *
   * This is the direct table look-up approach.
   *
   * Assume here the integer part of the fixed-point is <= 3.
   * More than 3 just not making much sense, makes no difference with
   * saturation followed by any of these activation functions.
   */

void csi_nn_activations_direct_q15(q15_t * data, uint16_t size,
                                    uint16_t int_width,
                                    csi_nn_activation_type type)
{
    uint16_t  i = size;
    q15_t    *pIn = data;
    q15_t    *pOut = data;
    uint16_t  shift_size = 8 + 3 - int_width;
    uint32_t  bit_mask = 0x7FF >> int_width;
    uint32_t  full_frac = bit_mask + 1;
    const q15_t *lookup_table;

    switch (type)
    {
    case CSKY_SIGMOID:
        lookup_table = sigmoidTable_q15;
        break;
    case CSKY_TANH:
    default:
        lookup_table = tanhTable_q15;
        break;
    }

    while (i)
    {
        q15_t     out;
        q15_t     in = *pIn++;
        q15_t     frac = (uint32_t) in & bit_mask;
        q15_t     value = lookup_table[(uint8_t)__SSAT(in >> shift_size, 8)];
        q15_t     value2 = lookup_table[(uint8_t)__SSAT(1 + (in >> shift_size), 8)];

        /* doing the interpolation here for better accuracy */
        out = ((q31_t)(full_frac - frac) * value + (q31_t) value2 * frac) >>
            shift_size;

        *pOut++ = out;
        i--;
    }

}

/**
 * @} end of Acti group
 */
