#include <vector>
#include <memory>

class Concept {
public:
  virtual ~Concept() = default;
  virtual std::unique_ptr<Concept> clone() const = 0;
  // virtual bool compare() = 0;
};

template <class T> class Model : public Concept {
public:
  Model(T const &model) : m_model(model) {}
  std::unique_ptr<Concept> clone() const override {
    return std::make_unique<Model<T>>(m_model);
  }

  T m_model;
};

using Histogram = std::vector<double>;
using EventList = std::vector<int>;

enum class ADSType { Histogram, EventList };

template <class T> ADSType getADSType() {
  throw std::runtime_error("Type not registered in ADS");
}

template <> ADSType getADSType<Histogram>() { return ADSType::Histogram; }
template <> ADSType getADSType<EventList>() { return ADSType::EventList; }

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

// template <class T> Histogram rebin(const T &unsupported) {
//  throw std::runtime_error("rebin does not support this type");
//}

// How can we avoid writing this for every algorithm?
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
  T m_model;
};

class Algorithm {
public:
  // void execute() { exec(); }
  // virtual void exec() = 0;
  template <class T>
  Algorithm(T object)
      : m_object(std::make_unique<AlgorithmModel<T>>(std::move(object))) {}
  Algorithm(const Algorithm &other) : m_object(other.m_object->clone()) {}

  ADSHandle execute(const ADSHandle &ws) {
    // How to handle multiple arguments and combinatoric explosion?
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
