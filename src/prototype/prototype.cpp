#include <cstdint>

#include <boost/tti/has_function.hpp>

#include "algorithms.h"
#include "data.h"
#include "instrument.h"
#include "metadata.h"

template <class Data> class Workspace {
public:
  using value_type = Data;
  Workspace() = default;
  template <class OtherData>
  explicit Workspace(const Workspace<OtherData> &other)
      : m_data(other.m_spectrumDefinitions.size()),
        m_spectrumDefinitions(other.m_spectrumDefinitions),
        m_spectrumNumbers(other.m_spectrumNumbers),
        m_spectrumInfo(other.m_spectrumInfo), m_logs(other.m_logs) {}
  typename std::vector<Data>::iterator begin() { return m_data.begin(); }
  typename std::vector<Data>::iterator end() { return m_data.end(); }
  size_t size() const { return m_data.size();}
  Data &operator[](const size_t i) { return m_data[i];}
  const Data &operator[](const size_t i) const { return m_data[i];}
  Logs &logs() { return m_logs; }
  const Logs &logs() const { return m_logs; }

//private:
  std::vector<Data> m_data;
  std::vector<SpectrumDefinition> m_spectrumDefinitions;
  std::vector<int32_t> m_spectrumNumbers;
  SpectrumInfo m_spectrumInfo;
  Logs m_logs;
};

// Part 2: Call apply functions
BOOST_TTI_HAS_FUNCTION(apply);
template <class Alg, class Enable, class Ws, class... Args> struct Apply;
template <class Alg, class Ws, class... Args>
struct Apply<Alg, typename std::enable_if<has_function_apply<
                      Alg, void, boost::mpl::vector<Logs &>>::value>::type,
             Ws, Args...> {
  static Ws run(const Alg &alg, Ws ws, const Args &... args) {
    alg.apply(ws.logs(), args...);
    return ws;
  }
};
template <class Alg, class Ws, class... Args>
struct Apply<Alg, typename std::enable_if<
                      !has_function_apply<
                          Alg, void, boost::mpl::vector<Logs &>>::value>::type,
             Ws, Args...> {
  static Ws run(const Alg &alg, Ws ws, const Args &... args) {
    // Can do multi-threading, basic logging, and cancellation here.
    for (auto &item : ws)
      alg.apply(item, args...);
    return ws;
  }
};

// As Apply, but output workspace type is different
template <class Alg, class Enable, class Ws, class... Args> struct ApplyMutate;
template <class Alg, class Ws, class... Args>
struct ApplyMutate<Alg, typename std::enable_if<
                      !has_function_apply<
                          Alg, void, boost::mpl::vector<Logs &>>::value>::type,
             Ws, Args...> {
  static auto run(const Alg &alg, const Ws &ws,
                                       const Args &... args) {
  using OutputItemType = decltype(alg.apply(ws[0], args...));
    Workspace<OutputItemType> out(ws);
    for(size_t i=0; i<ws.size(); ++i)
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
Ws callInstance(const Alg &alg, Ws ws, const Args &... args) {
  return Apply<Alg, void, Ws, Args...>::run(alg, ws, args...);
}

// Part 1: Create algorithm instance and forward to call helper.
template <class Alg, class Enable, class Ws, class... Args>
struct ConstructAndApply;
template <class Alg, class Ws, class... Args>
struct ConstructAndApply<
    Alg, typename std::enable_if<
             std::is_trivially_default_constructible<Alg>::value>::type,
    Ws, Args...> {
  static Ws run(Ws ws, const Args &... args) {
    Alg alg;
    // Alg constructor does not need arguments, pass all arguments to apply.
    return callInstance<Alg>(alg, std::move(ws), args...);
  }
};
template <class Alg, class Ws, class... Args>
struct ConstructAndApply<Alg, typename std::enable_if<std::is_constructible<
                                  Alg, Logs, Args...>::value>::type,
                         Ws, Args...> {
  static Ws run(Ws ws, const Args &... args) {
    Alg alg(ws.logs(), args...);
    // Alg constructor consumed the arguments, pass only workspace.
    return callInstance<Alg>(alg, std::move(ws));
  }
};

template <class Alg, class Ws, class... Args>
Ws call(Ws ws, const Args &... args) {
  return ConstructAndApply<Alg, void, Ws, Args...>::run(ws, args...);
}

int main() {
  // Transform workspace, keeping type (an just copy to output and modify).
  Workspace<Histogram> ws;
  ws = call<Scale>(std::move(ws), 2.3);
  ws = call<ClearLogs>(std::move(ws));
  Workspace<EventList> eventWs;
  eventWs = call<FilterByLogValue>(std::move(eventWs), "temp1", 274.0, 275.0);
  // Could use auto, this is just to make sure that we are getting the expected type.
  Workspace<Histogram> binned = callMutate<Rebin>(eventWs, BinEdges{});
}
