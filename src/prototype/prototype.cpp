#include <boost/tti/has_function.hpp>

#include "algorithms.h"
#include "workspace.h"

// Part 2: Call apply functions
BOOST_TTI_HAS_FUNCTION(apply);
template <class Alg, class Enable, class WsItem, class WsAux, class... Args>
struct Apply;
template <class Alg, class WsItem, class WsAux, class... Args>
struct Apply<Alg,
             typename std::enable_if<has_function_apply<
                 const Alg, void, boost::mpl::vector<Logs &>>::value>::type,
             WsItem, WsAux, Args...> {
  using Ws = Workspace<WsItem, WsAux>;
  static Ws run(const Alg &alg, Ws ws, const Args &... args) {
    alg.apply(ws.logs(), args...);
    return ws;
  }
};
template <class Alg, class WsItem, class WsAux, class... Args>
struct Apply<
    Alg,
    typename std::enable_if<has_function_apply<
        const Alg, void, boost::mpl::vector<WsItem &, Args...>>::value>::type,
    WsItem, WsAux, Args...> {
  using Ws = Workspace<WsItem, WsAux>;
  static Ws run(const Alg &alg, Ws ws, const Args &... args) {
    // Can do multi-threading, basic logging, and cancellation here.
    for (auto &item : ws)
      alg.apply(item, args...);
    return ws;
  }
};
template <class Alg, class WsItem, class WsAux, class... Args>
struct Apply<
    Alg, typename std::enable_if<has_function_apply<
             const Alg, typename std::result_of<decltype(&Alg::apply)(
                            const Alg, const WsItem &, const Args &...)>::type,
             boost::mpl::vector<const WsItem &, const Args &...>>::value>::type,
    WsItem, WsAux, Args...> {
  using Ws = Workspace<WsItem, WsAux>;
  using Out = Workspace<typename std::result_of<decltype(&Alg::apply)(
                            const Alg, const WsItem &, const Args &...)>::type,
                        WsAux>;
  static Out run(const Alg &alg, const Ws &ws, const Args &... args) {
    Out out(ws);
    for (size_t i = 0; i < ws.size(); ++i)
      out[i] = alg.apply(ws[i], args...);
    return out;
  }
};

// As Apply, but output workspace type is different
template <class Alg, class Enable, class Ws, class... Args> struct ApplyMutate;
template <class Alg, class Ws, class... Args>
struct ApplyMutate<Alg,
                   typename std::enable_if<
                       !has_function_apply<
                           Alg, void, boost::mpl::vector<Logs &>>::value>::type,
                   Ws, Args...> {
  static auto run(const Alg &alg, const Ws &ws, const Args &... args) {
    // Merge with Apply, if OutputItemType is void we work in-place?
    using OutputItemType = decltype(alg.apply(ws[0], args...));
    Workspace<OutputItemType> out(ws);
    for (size_t i = 0; i < ws.size(); ++i)
      out[i] = alg.apply(ws[i], args...);
    return out;
  }
};

template <class Alg, class Ws, class... Args>
auto callMutate(Ws ws, const Args &... args) {
  Alg alg;
  return ApplyMutate<Alg, void, Ws, Args...>::run(alg, ws, args...);
}

template <class Alg, class Ws, class... Args>
auto callInstance(const Alg &alg, Ws ws, const Args &... args) {
  return Apply<Alg, void, typename Ws::value_type, typename Ws::aux_type,
               Args...>::run(alg, std::forward<Ws>(ws), args...);
}

// Part 1: Create algorithm instance and forward to call helper.
template <class Alg, class Enable, class Ws, class... Args>
struct ConstructAndApply;
template <class Alg, class Ws, class... Args>
struct ConstructAndApply<
    Alg, typename std::enable_if<
             std::is_trivially_default_constructible<Alg>::value>::type,
    Ws, Args...> {
  static auto run(Ws ws, const Args &... args) {
    Alg alg;
    // Alg constructor does not need arguments, pass all arguments to apply.
    return callInstance<Alg>(alg, std::forward<Ws>(ws), args...);
  }
};
template <class Alg, class Ws, class... Args>
struct ConstructAndApply<Alg, typename std::enable_if<std::is_constructible<
                                  Alg, Logs, Args...>::value>::type,
                         Ws, Args...> {
  static auto run(Ws ws, const Args &... args) {
    Alg alg(ws.logs(), args...);
    // Alg constructor consumed the arguments, pass only workspace.
    return callInstance<Alg>(alg, std::forward<Ws>(ws));
  }
};

template <class Alg, class Ws, class... Args>
auto call(Ws &&ws, const Args &... args) {
  return ConstructAndApply<Alg, void, Ws, Args...>::run(std::forward<Ws>(ws),
                                                        args...);
}

int main() {
  static_assert(
      has_function_apply<
          const Rebin,
          typename std::result_of<decltype(&Rebin::apply)(
              const Rebin, const EventList &, const BinEdges &)>::type,
          boost::mpl::vector<const EventList &, const BinEdges &>>::value,
      "hmm");
  // Transform workspace, keeping type (an just copy to output and modify).
  Workspace<Histogram> ws;
  Workspace<Histogram, QInfo> qWs;
  ws = call<Scale>(std::move(ws), 2.3);
  qWs = call<Scale>(std::move(qWs), 2.3);
  ws = call<ClearLogs>(std::move(ws));
  Workspace<EventList> eventWs;
  eventWs = call<FilterByLogValue>(std::move(eventWs), "temp1", 274.0, 275.0);
  // Could use auto, this is just to make sure that we are getting the expected
  // type.
  Workspace<Histogram> binned = call<Rebin>(eventWs, BinEdges{});

  // auto fitResult = callMutate<Fit>(binned, Fit::Function{},
  // Fit::Parameters{});
}
