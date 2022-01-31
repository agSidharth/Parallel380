#pragma once

#include <stdint.h>

void merge(uint32_t *arr1,uint32_t size1,uint32_t *arr2,uint32_t size2,uint32_t* newarr);
void sequentialSort(uint32_t *arr,uint32_t size);
void ParallelSort(uint32_t *data, uint32_t n, int p);