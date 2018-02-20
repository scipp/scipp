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
  operator Logs &() { return m_logs; }

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

// Case 2 : Alg needs constructor arguments, apply is not static
BOOST_TTI_HAS_MEMBER_FUNCTION(apply);
template <class Alg, class Enable, class Ws, class... Args> struct Apply;
template <class Alg, class Ws, class... Args>
struct Apply<Alg, typename std::enable_if<
                      !has_member_function_apply<
                          Alg, void, boost::mpl::vector<Logs &>>::value>::type,
             Ws, Args...> {
  static Ws run(Ws ws, const Args &... args) {
    Alg alg(ws, args...);
    for (auto &item : ws)
      alg.apply(item);
    return ws;
  }
};

template <class Alg, class Ws, class... Args>
Ws call2(Ws ws, const Args &... args) {
  return Apply<Alg, void, Ws, Args...>::run(ws, args...);
}

int main() {
  Workspace<Histogram> ws;
  ws = call<Scale>(std::move(ws), 2.3);
  ws = call<ClearLogs>(std::move(ws));
  Workspace<EventList> eventWs;
  eventWs = call2<FilterByLogValue>(std::move(eventWs), "temp1", 274.0, 275.0);
}
