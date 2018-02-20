#include <cstdint>

#include <boost/tti/has_member_function.hpp>
#include <boost/tti/has_static_member_function.hpp>

#include "algorithms.h"
#include "data.h"
#include "instrument.h"
#include "metadata.h"

template <class Data> class Workspace {
public:
  typename std::vector<Data>::iterator begin() { return m_data.begin(); }
  typename std::vector<Data>::iterator end() { return m_data.end(); }
  Logs &logs() { return m_logs; }
  const Logs &logs() const { return m_logs; }

private:
  std::vector<Data> m_data;
  std::vector<SpectrumDefinition> m_spectrumDefinitions;
  std::vector<int32_t> m_spectrumNumbers;
  SpectrumInfo m_spectrumInfo;
  Logs m_logs;
};

// Case 1: No state, apply is static method of Alg
BOOST_TTI_HAS_STATIC_MEMBER_FUNCTION(apply);
template <class Alg, class Enable, class Ws, class... Args> struct ApplyStatic;
template <class Alg, class Ws, class... Args>
struct ApplyStatic<Alg,
                   typename std::enable_if<has_static_member_function_apply<
                       Alg, void, boost::mpl::vector<Logs &>>::value>::type,
                   Ws, Args...> {
  static Ws run(Ws ws, const Args &... args) {
    Alg::apply(ws.logs(), args...);
    return ws;
  }
};
template <class Alg, class Ws, class... Args>
struct ApplyStatic<Alg,
                   typename std::enable_if<
                       !has_static_member_function_apply<
                           Alg, void, boost::mpl::vector<Logs &>>::value>::type,
                   Ws, Args...> {
  static Ws run(Ws ws, const Args &... args) {
    // Can do multi-threading, basic logging, and cancellation here.
    for (auto &item : ws)
      Alg::apply(item, args...);
    return ws;
  }
};

template <class Alg, class Ws, class... Args>
Ws call(Ws ws, const Args &... args) {
  return ApplyStatic<Alg, void, Ws, Args...>::run(ws, args...);
}

// Case 2: Constructor needs arguments.
template <class Alg, class Enable, class Ws, class... Args> struct Construct;
template <class Alg, class Ws, class... Args>
struct Construct<Alg,
                 typename std::enable_if<
                     std::is_trivially_default_constructible<Alg>::value>::type,
                 Ws, Args...> {
  static Alg run(const Ws &ws, const Args &... args) {
    Alg alg;
    return alg;
  }
};
template <class Alg, class Ws, class... Args>
struct Construct<Alg, typename std::enable_if<std::is_constructible<
                          Alg, Logs, Args...>::value>::type,
                 Ws, Args...> {
  static Alg run(const Ws &ws, const Args &... args) {
    Alg alg(ws.logs(), args...);
    return alg;
  }
};

template <class Alg, class Ws, class... Args>
Ws callWithConstructor(Ws ws, const Args &... args) {
  const auto alg = Construct<Alg, void, Ws, Args...>::run(ws, args...);
  for (auto &item : ws)
    alg.apply(item);
  return ws;
}

int main() {
  Workspace<Histogram> ws;
  ws = call<Scale>(std::move(ws), 2.3);
  ws = call<ClearLogs>(std::move(ws));
  Workspace<EventList> eventWs;
  eventWs = callWithConstructor<FilterByLogValue>(std::move(eventWs), "temp1",
                                                  274.0, 275.0);
}
