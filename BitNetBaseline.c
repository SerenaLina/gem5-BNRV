#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <gem5/m5ops.h>

#define N 8192


void int8_matmul_kernel(int8_t* activations, int8_t* weights, int32_t* output) {
    for (int i = 0; i < N; i++) {
        int32_t sum = 0;

        int8_t* row_weights = &weights[i * N];

        for (int j = 0; j < N; j++) {

            sum += (int32_t)activations[j] * (int32_t)row_weights[j];
        }
        output[i] = sum;
    }
}

int main() {

    int8_t* weights = (int8_t*)malloc(N * N * sizeof(int8_t));
    int8_t* input_i = (int8_t*)malloc(N * sizeof(int8_t));
    int32_t* out = (int32_t*)malloc(N * sizeof(int32_t));


    for(int i = 0; i < N * N; i++) weights[i] = (int8_t)(rand() % 3 - 1);
    for(int i = 0; i < N; i++) input_i[i] = (int8_t)(rand() % 256 - 128);

    m5_reset_stats(0, 0);
    printf("Starting INT8 Baseline Simulation...\n");


    int8_matmul_kernel(input_i, weights, out);

    m5_dump_stats(0, 0);
    return 0;
}
