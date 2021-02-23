#include <scipp/dataset/dataset.h>

int main() {
  scipp::Dataset d;
  d + d;
}
