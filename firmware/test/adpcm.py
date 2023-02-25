# SPDX-License-Identifier:  Apache-2.0
#
# Copyright (c) 2023, Roman Turkin


import logging
import sys
import numpy as np

"""
This file contains methods for decoding ADPCM-coded data provided by Nordic in one of their reference examples.
Basically I took algorithm from dvi_adcpm.c found in https://www.nordicsemi.com/Products/Reference-designs/nRFready-Smart-Remote-3-for-nRF52-Series/Download?lang=en#infotabs
(archive nRF6939-SW) and converted the C code to python.

Unfortunately, I was unable to find a Python library that would've been capable of correctly interpreting this version of IMA ADPCM, so the easiest option for me has been
to take reference example and rewrite it in Python. It's also benefitial to have it in the form of code, as later I'd have to implement it in Swift and Java/Kotlin. 
"""

index_table = [
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
]

stepsize_table = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
]

def encode(input):
    in_size = int(len(input) / 2)

    output = bytearray([])
    valpred = np.int32(0)
    index = np.int32(0)
    step = stepsize_table[index]
    bufferstep = np.int32(1)
    outputbuffer = np.int32(0)

    for i in range(0, in_size):
        value = np.int16(input[2 * i] + (input[2 * i + 1] << 8))
        # print(value)
        
        # Step 1: compute difference
        diff = np.int32(value - valpred)
        sign = 0
        if diff < 0:
            sign = 8
        
        if (sign > 0):
            diff = -diff
        #Step 2: divide and clamp
        delta = 0
        vpdiff = (step >> 3)
        if diff >= step:
            delta |= 4
            diff -= step
            vpdiff += step
        step >>= 1
        if diff >= step:
            delta |= 2
            diff -= step
            vpdiff += step
        step >>= 1
        if diff >= step:
            delta |= 1
            vpdiff += step
        # Step 3: update previous value
        if sign > 0:
            valpred -= vpdiff
        else:
            valpred += vpdiff
        # Step 4: clamp prev val to 16 bits
        if valpred > 32767:
            valpred = 32767
        elif valpred < -32768:
            valpred = -32768
        # Step 5: assemble value, update index and step values
        delta |= sign

        index += index_table[delta]
        if index < 0:
            index = 0
        if index >= len(stepsize_table):
            index = len(stepsize_table) - 1
        step = stepsize_table[index]

        # step 6
        if bufferstep > 0:
            outputbuffer = (delta << 4) & 0xF0
        else:
            output.append((delta&0xF) | outputbuffer)
        if bufferstep != 0:
            bufferstep = 0
        else:
            bufferstep = 1

    if bufferstep > 0:
        output.append(outputbuffer)

    return output


# TODO: fix handling of the first 3 bytes (it regulates the initial state of the data by setting correct valpred and index values)
def adpcm_decode(input):
    # first 3 bytes contain the information regarding the initial state of the coding
    # todo: implement

    valpred = 0
    delta = 0
    index = 0

    output = bytearray([])
    step = stepsize_table[index]

    bufferstep = 0
    inputbuffer = 0

    in_size = len(input)
    in_size = (in_size - 3) * 2
    out_size = in_size * 2

    i = 0
    while in_size > 0:
        #step 1: get the delta value
        if bufferstep > 0:
            delta = inputbuffer & 0xF
        else:
            inputbuffer = input[i]
            delta = (inputbuffer >> 4) & 0xf
            i += 1
        bufferstep = not bufferstep

        #step 2: find the new index value
        index += index_table[delta]
        if index < 0:
            index = 0
        if index > 88:
            index = 88
        
        #step 3: separate sign and magnitude
        sign = delta & 8
        delta = delta & 7

        #step 4: compute difference and new predicted value
        vpdiff = step >> 3
        if delta & 4 > 0:
            vpdiff += step
        if delta & 2 > 0:
            vpdiff += (step>>1)
        if delta & 1 > 0:
            vpdiff += (step >> 2)

        if (sign > 0):
            valpred -= vpdiff
        else:
            valpred += vpdiff
        
        #step 5 - clamp outputvalue
        if valpred > 32768:
            valpred = 32768
        elif valpred < -32768:
            valpred = 32768

        #step 6: update step value
        step = stepsize_table[index]

        # step 7: output value
        output.append(valpred & 0xff)
        output.append((valpred >> 8) & 0xff)

        in_size -= 1

    return output
