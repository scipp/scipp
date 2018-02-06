#include <vector>
#include <memory>

using Histogram = std::vector<double>;
using EventList = std::vector<int>;

enum class ADSType { Histogram, EventList };

template <class T> ADSType getADSType() {
  throw std::runtime_error("Type not registered in ADS");
}

template <> ADSType getADSType<Histogram>() { return ADSType::Histogram; }
template <> ADSType getADSType<EventList>() { return ADSType::EventList; }

// Concept, Model, and ADSHandle can hold arbitrary type. This is probably not
// actually relevant here, could be done in different way as well.
class Concept {
public:
  virtual ~Concept() = default;
  virtual std::unique_ptr<Concept> clone() const = 0;
};

template <class T> class Model : public Concept {
public:
  Model(T const &model) : m_model(model) {}
  std::unique_ptr<Concept> clone() const override {
    return std::make_unique<Model<T>>(m_model);
  }

  T m_model;
};

class ADSHandle {
public:
  template <class T>
  ADSHandle(T object)
      : m_type(getADSType<T>()),
        m_object(std::make_unique<Model<T>>(std::move(object))) {}
  ADSHandle(const ADSHandle &other) : m_object(other.m_object->clone()) {}

  template <class T> const T &cast() const {
    return dynamic_cast<Model<T> &>(*m_object).m_model;
  }

  ADSType type() const { return m_type; }

private:
  ADSType m_type;
  std::unique_ptr<Concept> m_object;
};

Histogram rebin(const Histogram &in) {
  auto out = in;
  out.resize(in.size() / 2);
  return out;
}

Histogram rebin(const EventList &in) { return Histogram{1.1, 2.2, 3.3}; }

// If we hold arbitrary type in ADS, we would need a way to get the actual type
// such that we can call the right overload. How can we avoid writing this for
// every algorithm?
ADSHandle rebin(const ADSHandle &ws) {
  switch (ws.type()) {
  case ADSType::Histogram:
    return rebin(ws.cast<Histogram>());
  case ADSType::EventList:
    return rebin(ws.cast<EventList>());
  default:
    throw std::runtime_error("rebin does not support this type");
  }
}

/// AlgorithmConcept, AlgorithmModel, and Algorithm provide a way to have a
/// unified workspace-type-to-overload resolution without duplicating code.
class AlgorithmConcept {
public:
  virtual ~AlgorithmConcept() = default;
  virtual std::unique_ptr<AlgorithmConcept> clone() const = 0;
  virtual ADSHandle exec(const Histogram &in) const = 0;
  virtual ADSHandle exec(const EventList &in) const = 0;
};

template <class T> class AlgorithmModel : public AlgorithmConcept {
public:
  AlgorithmModel(T const &model) : m_model(model) {}
  std::unique_ptr<AlgorithmConcept> clone() const override {
    return std::make_unique<AlgorithmModel<T>>(m_model);
  }

  ADSHandle exec(const Histogram &in) const override {
    return m_model.exec(in);
  }
  ADSHandle exec(const EventList &in) const override {
    return m_model.exec(in);
  }

private:
  // Could handle multiple arguments along the lines of this:
  /*
  ADSHandle doExec() {
    // get all properties, cast, build parameter pack?
  }
  template <class... Args>
  ADSHandle doExec(Args &&... args, Properties &props) {
    if (props.empty())
      m_model.exec(args...);
    else {
      auto prop = props.pop();
      switch (prop.type()) {
      // This leads to a horrible combinatoric explosion.
      // What if we restrict it to the supported types of m_model? How?
      case ADSType::Histogram:
        doExec(args..., prop.cast<Histogram>, props);
        break;
      }
      // other cases here
    }
  }
  */

  T m_model;
};

class Algorithm {
public:
  template <class T>
  Algorithm(T object)
      : m_object(std::make_unique<AlgorithmModel<T>>(std::move(object))) {}
  Algorithm(const Algorithm &other) : m_object(other.m_object->clone()) {}

  ADSHandle execute(const ADSHandle &ws) {
    // How to handle multiple arguments and combinatoric explosion? Multiple
    // arguments could be supported by converting a vector of properties into a
    // parameter pack (see doExec above), but would still suffer from
    // combinatoric explosion (potentially leading to long compile times or
    // large binaries?).
    switch (ws.type()) {
    case ADSType::Histogram:
      return m_object->exec(ws.cast<Histogram>());
    case ADSType::EventList:
      return m_object->exec(ws.cast<EventList>());
    default:
      throw std::runtime_error("rebin does not support this type");
    }
  }

private:
  std::unique_ptr<AlgorithmConcept> m_object;
};

// Via the mechanism provided by Algorithm we can now write classes like this,
// drop them into an Algorithm, and call them by passing any ADSHandle to
// Algorithm::execute.
class Rebin {
public:
  ADSHandle exec(const Histogram &in) const { return rebin(in); }
  ADSHandle exec(const EventList &in) const { return rebin(in); }
};

int main() {
  // We should actually always avoid working with ADSHandle. Do not repeat
  // mistakes from current way of working!
  auto ws2D = ADSHandle(Histogram{1, 2, 3});
  auto wsEvent = ADSHandle(EventList{10, 20, 30});
  auto result1 = rebin(ws2D);
  auto result2 = rebin(wsEvent);
  Algorithm alg(Rebin{});
  auto result3 = alg.execute(ws2D);
  auto result4 = alg.execute(wsEvent);
}
