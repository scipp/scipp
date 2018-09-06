/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef CALL_WRAPPERS_H
#define CALL_WRAPPERS_H

#include <boost/tti/has_function.hpp>

#include "dataset.h"
#include "dataset_iterator.h"
#include "function_traits.h"

class Logs {};

template <class Alg, class Callable, int ArgCount> struct ApplyToItems {};

template <class Alg, class Callable>
struct ApplyToItems<Alg, Callable, 1> {
static void run(const Alg &alg, Callable f, Dataset &d) {
  auto &col = d.get<std::vector<std::remove_reference_t<
      typename function_traits<Callable>::template argument<0>::type>>>();
  for(auto &item : col)
    f(item);
}
};

template <class Alg, class Callable>
struct ApplyToItems<Alg, Callable, 2> {
  template <int N>
  using Arg = std::remove_reference_t<
      typename function_traits<Callable>::template argument<N>::type>;
  static void run(const Alg &alg, Callable f, Dataset &d) {
    DatasetIterator<Arg<0>, Arg<1>> it(d);
    // TODO iterate everything here, accessing single element just for testing.
    f(it.template get<Arg<0>>(), it.template get<Arg<1>>());
}
};

template <class Alg, class Callable>
void call(const Alg &alg, Callable f, Dataset &d) {
  ApplyToItems<Alg, Callable, function_traits<Callable>::arity>::run(alg, f, d);
}

// Part 2: Call apply functions
BOOST_TTI_HAS_FUNCTION(apply);
template <class Alg, class Enable, class... Args> struct Apply;
template <class Alg, class... Args>
struct Apply<
    Alg,
    typename std::enable_if<has_function_apply<
        const Alg, void, boost::mpl::vector<Dataset &, Args...>>::value>::type,
    Args...> {
  static Dataset run(const Alg &alg, Dataset dataset, const Args &... args) {
    alg.apply(dataset, args...);
    return dataset;
  }
};
template <class Alg, class... Args>
struct Apply<
    Alg,
    typename std::enable_if<!has_function_apply<
        const Alg, void, boost::mpl::vector<Dataset &, Args...>>::value>::type,
    Args...> {
  static Dataset run(const Alg &alg, Dataset dataset, const Args &... args) {
    // TODO
    // 1. how do we run the runtime check for ColumnArgs (in case we want to try getting items/iterator)
    // 2. where to we get ColumnArgs (generic problem if there are overloads!)?
    //alg.apply(dataset.get<ColumnArgs>()..., args...);

    //DatasetIterator<ColumnArgs...> view(dataset);
    //for(auto &item : view) // This type of iteration is rather slow, support special case for single (or matching?) columns?
      // TODO need to make sure that all this can get inlined?
      //alg.apply(item.get<ColumnArgs>()..., args...);
    
    //call(alg, &Alg::apply, dataset);
    //ApplyToItems<&Alg::apply>::run(alg, dataset);
    call(alg, &Alg::apply, dataset);
    return dataset;
  }
};

/*
template <class Alg, class... Args>
struct Apply<
    Alg,
    typename std::enable_if<has_function_apply<
        const Alg, void, boost::mpl::vector<int &, Args...>>::value>::type,
    Args...> {
  static Dataset run(const Alg &alg, Dataset dataset, const Args &... args) {
    fprintf(stderr, "using int overload\n");
    return dataset;
  }
};
*/

/*
template <class Alg, class... Args>
struct Apply<
    Alg, typename std::enable_if<has_function_apply<
             const Alg, decltype(std::declval<const Alg>().apply(
                            std::declval<const WsItem &>(),
                            std::declval<const Args &>()...)),
             boost::mpl::vector<const WsItem &, const Args &...>>::value>::type,
    Args...> {
  using Ws = Workspace<WsItem, WsAux>;
  using Out = Workspace<decltype(std::declval<const Alg>().apply(
                            std::declval<const WsItem &>(),
                            std::declval<const Args &>()...)),
                        WsAux>;
  static Out run(const Alg &alg, const Ws &ws, const Args &... args) {
    Out out(ws);
    for (size_t i = 0; i < ws.size(); ++i)
      out[i] = alg.apply(ws[i], args...);
    return out;
  }
  static Out run(const Alg &alg, const Ws &ws, const IndexSet &indexSet,
                 const Args &... args) {
    Out out(ws, indexSet);
    for (size_t i = 0; i < indexSet.size(); ++i)
      out[i] = alg.apply(ws[indexSet[i]], args...);
    return out;
  }
};
*/

template <class Alg, class... Args>
auto callInstance(const Alg &alg, Dataset dataset, const Args &... args) {
  // Need to select correct Apply, cases to distinguish are:
  // - apply to single item (-> if getting column fails, try getting std::vector<item> (maybe use helper template column_type<T>, to support special columns that are not vectors))
  // - apply to column (-> use Dataset::hasColumn<T>? how can we deal with columns of duplicate type? must provide selection? let user select column?! apply to all??)
  // - apply to item obtained from typed iterator (-> if getting column and column of items fails? use templated helper iterator_type<T>?)
  // - apply to Dataset (-> enable_if apply accepts Dataset)
  return Apply<Alg, void, Args...>::run(alg, std::move(dataset), args...);
}
//template <class Alg, class... Args>
//auto callInstance(const Alg &alg, Dataset dataset, const IndexSet &indexSet,
//                  const Args &... args) {
//  return Apply<Alg, void, typename std::remove_reference<Ws>::type::value_type,
//               typename std::remove_reference<Ws>::type::aux_type,
//               Args...>::run(alg, std::forward<Ws>(ws), indexSet, args...);
//}

// Part 1: Create algorithm instance and forward to call helper.
template <class Alg, class Enable, class... Args>
struct ConstructAndApply;
template <class Alg, class... Args>
struct ConstructAndApply<
    Alg, typename std::enable_if<
             std::is_trivially_default_constructible<Alg>::value>::type,
    Args...> {
  static auto run(Dataset dataset, const Args &... args) {
    Alg alg;
    // Alg constructor does not need arguments, pass all arguments to apply.
    return callInstance<Alg>(alg, std::move(dataset), args...);
  }
  //static auto run(Dataset dataset, const IndexSet &indexSet, const Args &... args) {
  //  Alg alg;
  //  // Alg constructor does not need arguments, pass all arguments to apply.
  //  return callInstance<Alg>(alg, std::move(dataset), indexSet, args...);
  //}
};
template <class Alg, class Arg1, class... Args>
struct ConstructAndApply<Alg, typename std::enable_if<std::is_constructible<
                                  Alg, Arg1, Args...>::value>::type,
                         Arg1, Args...> {
  static auto run(Dataset dataset, const Arg1 &arg1, const Args &... args) {
    Alg alg(arg1, args...);
    // Alg constructor consumed the arguments, pass only workspace.
    return callInstance<Alg>(alg, std::move(dataset));
  }
  //static auto run(Dataset dataset, const IndexSet &indexSet, const Arg1 &arg1,
  //                const Args &... args) {
  //  Alg alg(arg1, args...);
  //  // Alg constructor consumed the arguments, pass only workspace.
  //  return callInstance<Alg>(alg, std::move(dataset), indexSet);
  //}
};
//template <class Alg, class... Args>
//struct ConstructAndApply<Alg, typename std::enable_if<std::is_constructible<
//                                  Alg, Logs, Args...>::value>::type,
//                         Args...> {
//  static auto run(Dataset dataset, const Args &... args) {
//    Alg alg(ws.logs(), args...);
//    // Alg constructor consumed the arguments, pass only workspace.
//    return callInstance<Alg>(alg, std::move(dataset));
//  }
//};

template <class Alg, class... Args>
auto call(Dataset dataset, const Args &... args) {
  return ConstructAndApply<Alg, void, Args...>::run(std::move(dataset),
                                                    args...);
}
//template <class Alg, class Ws, class... Args>
//auto call(Ws &&ws, const IndexSet &indexSet, const Args &... args) {
//  return ConstructAndApply<Alg, void, Ws, Args...>::run(std::forward<Ws>(ws),
//                                                        indexSet, args...);
//}

#endif // CALL_WRAPPERS_H
