#include <vector>
#include <memory>

#include <boost/any.hpp>

using Histogram = std::vector<double>;
using EventList = std::vector<int>;

enum class ADSType { Histogram, EventList };

template <class T> ADSType getADSType() {
  throw std::runtime_error("Type not registered in ADS");
}

template <> ADSType getADSType<Histogram>() { return ADSType::Histogram; }
template <> ADSType getADSType<EventList>() { return ADSType::EventList; }

Histogram rebin(const Histogram &in) {
  auto out = in;
  out.resize(in.size() / 2);
  return out;
}

// Histogram rebin(const EventList &in) { return Histogram{1.1, 2.2, 3.3}; }

class ADSHandle {
public:
  template <class T> ADSHandle(T &&object) : m_object(std::move(object)) {}
  template <class T> operator const T &() const {
    return boost::any_cast<const T &>(m_object);
  }

private:
  boost::any m_object;
};

int main() {
  boost::any ws(Histogram{});
  if (ws.type() == typeid(Histogram))
    rebin(boost::any_cast<Histogram>(ws));
  // if (ws.type() == typeid(EventList))
  //  rebin(boost::any_cast<EventList>(ws));

  ADSHandle ws2(Histogram{});
  // Overload resolution does not work.
  rebin(ws2);
  // How can we make code like this work?
  // rebin(ads.get("ws"))? with pybind11 the correct type and overload is
  // decuded automatically.
}
