#include <vector>
#include <pybind11/pybind11.h>

namespace py = pybind11;

using Histogram = double;

Histogram rebin(const Histogram &data) { return data; }
Histogram convertUnits(const Histogram &data, const double aux) {
  return data + aux;
}
Histogram convertUnits(const Histogram &data, const int aux) {
  return data * aux;
}

PYBIND11_MODULE(composition_exports, m) {
  m.def("rebin", py::overload_cast<const Histogram &>(&rebin));
  m.def("convertUnits",
        py::overload_cast<const Histogram &, const double>(&convertUnits),
        py::arg("histogram"), py::arg("offset"));
  m.def("convertUnits",
        py::overload_cast<const Histogram &, const int>(&convertUnits),
        py::arg("histogram"), py::arg("scale"));
}
