#pragma once

//std::vector<int> array2vec (int a[], size_t a_size);
#define array2vec(a) (std::vector<int>(std::begin(a), std::end(a)))

#define ptr2vec(a, a_size) (std::vector<int>(a, a + (a_size)))
