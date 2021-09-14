#include "serac/physics/utilities/functional/array.hpp"

__global__ void fill_kernel(axom::GPUView< double, 2 > values) {
  values(threadIdx.x, blockIdx.x) = threadIdx.x + blockIdx.x;
} 

int main() {

  axom::GPUArray< double, 2 > my_array(32, 32);

  fill_kernel<<<32,32>>>(view(my_array));

  axom::CPUArray< double, 2 > my_array_h = my_array;

  for (int i = 0; i < 4; i++) {
    std::cout << my_array_h(4, i) << std::endl;
  }

}
