#include <cstdint>

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

private:
  std::vector<Data> m_data;
  std::vector<SpectrumDefinition> m_spectrumDefinitions;
  std::vector<int32_t> m_spectrumNumbers;
  SpectrumInfo m_spectrumInfo;
  Logs m_logs;
};

BOOST_TTI_HAS_STATIC_MEMBER_FUNCTION(apply);

template <class Alg, class Enable, class Ws, class... Args> struct Call;
template <class Alg, class Ws, class... Args>
struct Call<Alg, typename std::enable_if<has_static_member_function_apply<
                     Alg, void, boost::mpl::vector<Logs &>>::value>::type,
            Ws, Args...> {
  static Ws apply(Ws ws, const Args &... args) {
    Alg::apply(ws.logs(), args...);
    return ws;
  }
};
template <class Alg, class Ws, class... Args>
struct Call<Alg, typename std::enable_if<
                     !has_static_member_function_apply<
                         Alg, void, boost::mpl::vector<Logs &>>::value>::type,
            Ws, Args...> {
  static Ws apply(Ws ws, const Args &... args) {
    for (auto &item : ws)
      Alg::apply(item, args...);
    return ws;
  }
};

// template <class Alg, class Ws,
//           typename = typename
//           std::enable_if<has_static_member_function_apply<
//               Alg, void, boost::mpl::vector<Logs &>>::value>::type,
//           class... Args>
// Ws call(Ws ws, const Args &... args) {
//   Alg::apply(ws.logs(), args...);
//   return ws;
// }

// Passing ws by value, so in-place operation could be done using std::move.
// template <class Alg, class Ws,
//           typename = typename std::enable_if<
//               !has_static_member_function_apply<
//                   Alg, void, boost::mpl::vector<Logs &>>::value>::type,
//           class... Args>
// Ws call(Ws ws, const Args &... args) {
//   for (auto &item : ws)
//     Alg::apply(item, args...);
//   return ws;
// }

template <class Alg, class Ws, class... Args>
Ws call(Ws ws, const Args &... args) {
  return Call<Alg, void, Ws, Args...>::apply(ws, args...);
}

int main() {
  Workspace<Histogram> ws;
  ws = call<Scale>(std::move(ws), 2.3);
  ws = call<ClearLogs>(std::move(ws));
}
