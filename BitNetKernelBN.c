#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <gem5/m5ops.h>

#define N 8192


#define BITNET_STEP_HW(weights_byte, act_ptr, sum_accumulator) do { \
    uint32_t rs1_val = *((uint32_t*)(act_ptr)); \
    uint32_t rs2_val = (uint32_t)(weights_byte); \
    uint32_t rd_raw; \
    asm volatile ( \
        "mv x6, %1\n\t" \
        "mv x7, %2\n\t" \
        ".word 0x0073028b\n\t" \
        "mv %0, x5\n\t" \
        : "=r" (rd_raw) \
        : "r" (rs1_val), "r" (rs2_val) \
        : "x5", "x6", "x7", "memory" \
    ); \
    sum_accumulator += rd_raw; \
} while(0)


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
    float rms = sqrtf(sum / n + 1e-6f);
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
    float scale = (max_abs > 1e-8f) ? (127.0f / max_abs) : 0.0f;
    for (int i = 0; i < N; i++) {
        output[i] = (int8_t)roundf(input[i] * scale);
    }
}


void bitnet_matmul_kernel(int8_t* activations, int8_t* packed_weights, int32_t* output) {
    for (int i = 0; i < N; i++) {
        int32_t row_sum = 0;
        uint8_t* row_weights = (uint8_t*)&packed_weights[i * (N / 4)];

        for (int j = 0; j < N / 4; j++) {
            BITNET_STEP_HW(row_weights[j], &activations[j * 4], row_sum);
        }
        output[i] = row_sum;
    }
}

int main() {
    size_t total_weights_bytes = (N * N) / 4;

    int8_t* weights = (int8_t*)malloc(total_weights_bytes);
    int8_t* input_i = (int8_t*)malloc(N * sizeof(int8_t));
    float* input   = (float*)malloc(N * sizeof(float));
    float* input_n = (float*)malloc(N * sizeof(float));
    int32_t* out    = (int32_t*)malloc(N * sizeof(int32_t));


    for (int i = 0; i < total_weights_bytes; i++) {
        weights[i] = packed_fourweights(1, 3, 0, 2);
    }
    for (int i = 0; i < N; i++) {
        input[i] = 0.25f;
    }
    rms_norm(input, input_n, N);
    int_quant(input_n, input_i);
    printf("Starting BitNet Hardware Kernel Simulation (N=%d)...\n", N);
    m5_reset_stats(0, 0);
    bitnet_matmul_kernel(input_i, weights, out);
    m5_dump_stats(0, 0);
    int64_t final_checksum = 0;
    for (int i = 0; i < N; i++) {
        final_checksum += out[i];
    }
    printf("Output Checksum: %ld\n", final_checksum);
    free(weights); free(input_i); free(input); free(input_n); free(out);
    return 0;
}
