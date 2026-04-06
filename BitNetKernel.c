#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <gem5/m5ops.h>

#define N 8192
static const int8_t SCALE_LUT[4] = {0, 1, -1, -2};

#define BITNET_STEP(bit, idx) { \
    uint8_t w_val = (weight_byte >> (bit)) & 0x03; \
    sum += (int32_t)act_ptr[idx] * SCALE_LUT[w_val]; \
}

int8_t packed_fourweights(int8_t w0, int8_t w1, int8_t w2, int8_t w3) {
    int8_t bytes = 0;
    bytes |= (w0 & 0x03) << 0;
    bytes |= (w1 & 0x03) << 2;
    bytes |= (w2 & 0x03) << 4;
    bytes |= (w3 & 0x03) << 6;
    return bytes;
}

void rms_norm(float* input, float* output, int n) {
    float sum = 0;
    for (int i = 0; i < n; i++) sum += input[i] * input[i];
    float rms = sqrtf(sum / (float)n + 1e-6f);
    for (int i = 0; i < n; i++) {
        output[i] = input[i] / rms;
    }
}

void int_quant(float* input, int8_t* output) {
    float max_abs = 0.0f;
    for (int i = 0; i < N; i++) {
        float val = fabsf(input[i]);
        if (val > max_abs) max_abs = val;
    }
    float scale = (max_abs > 1e-6f) ? (127.0f / max_abs) : 0.0f;
    for (int i = 0; i < N; i++) {
        output[i] = (int8_t)roundf(input[i] * scale);
    }
}

void bitnet_matmul_kernel(int8_t* activations, uint8_t* packed_weights, int32_t* output) {
    for (int i = 0; i < N; i++) {
        int32_t sum = 0;
        uint8_t* row_weights = &packed_weights[i * (N / 4)];

        for (int j = 0; j < N / 4; j++) {
            uint8_t weight_byte = row_weights[j];
            int8_t* act_ptr = &activations[j * 4];

            BITNET_STEP(0, 0)
            BITNET_STEP(2, 1)
            BITNET_STEP(4, 2)
            BITNET_STEP(6, 3)
        }
        output[i] = sum;
    }
}

int main() {
    size_t total_weights_bytes = (N * N) / 4;
    uint8_t* weights = (uint8_t*)malloc(total_weights_bytes);
    int8_t* input_i = (int8_t*)malloc(N * sizeof(int8_t));
    float* input = (float*)malloc(N * sizeof(float));
    float* input_n = (float*)malloc(N * sizeof(float));
    int32_t* out = (int32_t*)malloc(N * sizeof(int32_t));

    for(size_t i = 0; i < total_weights_bytes; i++) {
        weights[i] = (uint8_t)packed_fourweights(1, 3, 0, 2);
    }
    for (int i = 0; i < N; i++) {
        input[i] = 0.25f;
    }

    rms_norm(input, input_n, N);
    int_quant(input_n, input_i);

    printf("Starting BitNet Kernel Simulation...\n");

    m5_reset_stats(0, 0);
    bitnet_matmul_kernel(input_i, weights, out);
    m5_dump_stats(0, 0);

    int64_t final_checksum = 0;
    for (int i = 0; i < N; i++) {
        final_checksum += out[i];
    }
    printf("Output Checksum: %ld\n", (long)final_checksum);

    free(weights);
    free(input_i);
    free(input);
    free(input_n);
    free(out);
    return 0;
}
