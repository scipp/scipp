//
// Created by igudich on 10/21/19.
//

#ifndef SCIPP_BINDER_H
#define SCIPP_BINDER_H

#include <functional>
#include <set>

namespace bind {

enum class Priority {
  NativeScipp, // no implicit conversions to other types available in pybind11
  PythonList,  // no implicit conversions to other types available in pybind11
  NumpyArray,  //
  PythonListNamedValues, // should be here to split cases Variable([Dims],
                         // [Shape]) and Variable([Dims], [Values]), the values
                         // should be named for proper result
  NumpyBuffer,  // This is used for treating numpy native arithmetic types, e.g.
                // numpy.float32, but numpy.array is also a buffer
  NativePython, // Should be after NumpyBuffer not to collide with python native
                // types
  PythonObject  // The most general fallback
};

template <class ObjToBindType> class Binder {
  struct PriorityFunction {
    std::function<void(ObjToBindType &obj)> bindFunction;
    Priority priority;
    bool operator<(const PriorityFunction &other) const {
      return priority < other.priority;
    }
  };

public:
  template <class... TS>
  Binder(TS &&... args) : object(std::forward<TS>(args)...){};
  void append(PriorityFunction func) { bindings.emplace(std::move(func)); };
  ObjToBindType &bind() {
    for (auto &&bnd : bindings)
      bnd.bindFunction(object);
    bindings.clear();
    return object;
  };

private:
  ObjToBindType object;
  std::multiset<PriorityFunction> bindings;
};

} // namespace bind

#endif // SCIPP_BINDER_H
