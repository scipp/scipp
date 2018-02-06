#include <pybind11/pybind11.h>

namespace py = pybind11;

int rebin(int i, int j) { return 42; }
int rebin(double d, int j) { return 43; }

PYBIND11_MODULE(rebin, m) {
  m.doc() = "rebin";
  m.def("rebin", py::overload_cast<double, int>(&rebin), "rebins data");
  m.def("rebin", py::overload_cast<int, int>(&rebin), "rebins data");
}
