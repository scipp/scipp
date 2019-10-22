//
// Created by igudich on 10/21/19.
//

#ifndef SCIPP_BINDER_H
#define SCIPP_BINDER_H

#include <functional>
#include <set>

namespace bind {

enum class Priority {
  NativeScipp, NumpyArray, PythonList, NumpyBuffer, NativePython, PythonObject
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
  Binder(TS &&... args)
      : object(std::forward<TS>(args)...) {}
  ;
  void append(PriorityFunction func) { bindings.emplace(std::move(func)); }
  ;
  ObjToBindType &bind() {
    for (auto &&bnd : bindings)
      bnd.bindFunction(object);
    bindings.clear();
    return object;
  }
  ;

private:
  ObjToBindType object;
  std::multiset<PriorityFunction> bindings;
};

} // bind

#endif //SCIPP_BINDER_H
