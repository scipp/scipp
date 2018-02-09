#include <algorithm>
#include <numeric>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include "range/v3/all.hpp"

/*
using Histogram = std::vector<double>;
using SpectrumNumber = int32_t;

class Spectrum;
class WorkspaceIterator;
class WorkspaceConstIterator;

class Workspace {
public:
  // using value_type = Spectrum;
  Workspace(const size_t size) : m_histograms(size), m_spectrumNumbers(size) {
    std::iota(m_spectrumNumbers.begin(), m_spectrumNumbers.end(), 1);
  }

  // SOA accessors
  Histogram &histogram(const size_t i) { return m_histograms[i]; }
  const Histogram &histogram(const size_t i) const { return m_histograms[i]; }
  SpectrumNumber &spectrumNumber(const size_t i) {
    return m_spectrumNumbers[i];
  }
  const SpectrumNumber &spectrumNumber(const size_t i) const {
    return m_spectrumNumbers[i];
  }

  // AOS accessors
  Spectrum operator[](const size_t i);
  WorkspaceIterator begin();
  WorkspaceConstIterator begin() const;
  WorkspaceIterator end();
  WorkspaceConstIterator end() const;
  void push_back(const Spectrum &spectrum);

private:
  std::vector<Histogram> m_histograms;
  std::vector<SpectrumNumber> m_spectrumNumbers;
};

class WorkspaceIterator;

class Spectrum {
public:
  Spectrum(Workspace &workspace, const size_t index)
      : m_workspace(workspace), m_index(index) {}

  // Spectrum(const Spectrum &) = delete;
  // Spectrum(Spectrum &&) = default;
  // const Spectrum &operator=(const Spectrum &other) {
  //   histogram() = other.histogram();
  //   spectrumNumber() = other.spectrumNumber();
  // }

  Histogram &histogram() { return m_workspace.histogram(m_index); }
  const Histogram &histogram() const { return m_workspace.histogram(m_index); }
  SpectrumNumber &spectrumNumber() {
    return m_workspace.spectrumNumber(m_index);
  }
  const SpectrumNumber &spectrumNumber() const {
    return m_workspace.spectrumNumber(m_index);
  }

  friend class Workspace;
  friend class WorkspaceIterator;
  friend class WorkspaceConstIterator;

private:
  Workspace &m_workspace;
  size_t m_index;
};

class ConstSpectrum {
public:
  ConstSpectrum(ConstWorkspace &workspace, const size_t index)
      : m_workspace(workspace), m_index(index) {}

  // Spectrum(const Spectrum &) = delete;
  // Spectrum(Spectrum &&) = default;
  // const Spectrum &operator=(const Spectrum &other) {
  //   histogram() = other.histogram();
  //   spectrumNumber() = other.spectrumNumber();
  // }

  const Histogram &histogram() const { return m_workspace.histogram(m_index); }
  const SpectrumNumber &spectrumNumber() const {
    return m_workspace.spectrumNumber(m_index);
  }

  friend class Workspace;
  friend class WorkspaceIterator;
  friend class WorkspaceConstIterator;

private:
  ConstWorkspace &m_workspace;
  size_t m_index;
};

class WorkspaceIterator
    : public boost::iterator_facade<WorkspaceIterator, Spectrum &,
                                    boost::bidirectional_traversal_tag> {
public:
  WorkspaceIterator(Workspace &workspace, const size_t index)
      : m_item(workspace, index){};

private:
  friend class boost::iterator_core_access;

  void increment() { ++m_item.m_index; }

  bool equal(const WorkspaceIterator &other) const {
    return m_item.m_index == other.m_item.m_index;
  }

  Spectrum &dereference() { return m_item; }

  void decrement() { --m_item.m_index; }

  void advance(int64_t delta) { m_item.m_index += delta; }

  uint64_t distance_to(const WorkspaceIterator &other) const {
    return static_cast<uint64_t>(other.m_item.m_index) -
           static_cast<uint64_t>(m_item.m_index);
  }

  Spectrum m_item;
};

class WorkspaceConstIterator
    : public boost::iterator_facade<WorkspaceConstIterator, const Spectrum &,
                                    boost::bidirectional_traversal_tag> {
public:
  WorkspaceConstIterator(const Workspace &workspace, const size_t index)
      : m_item(workspace, index){};

private:
  friend class boost::iterator_core_access;

  void increment() { ++m_item.m_index; }

  bool equal(const WorkspaceConstIterator &other) const {
    return m_item.m_index == other.m_item.m_index;
  }

  const Spectrum &dereference() const { return m_item; }

  void decrement() { --m_item.m_index; }

  void advance(int64_t delta) { m_item.m_index += delta; }

  uint64_t distance_to(const WorkspaceConstIterator &other) const {
    return static_cast<uint64_t>(other.m_item.m_index) -
           static_cast<uint64_t>(m_item.m_index);
  }

  Spectrum m_item;
};

WorkspaceIterator Workspace::begin() { return WorkspaceIterator(*this, 0); }
WorkspaceConstIterator Workspace::begin() const {
  return WorkspaceConstIterator(*this, 0);
}
WorkspaceIterator Workspace::end() {
  return WorkspaceIterator(*this, m_histograms.size());
}
WorkspaceConstIterator Workspace::end() const {
  return WorkspaceConstIterator(*this, m_histograms.size());
}

Spectrum Workspace::operator[](const size_t i) {
  // How can we make this thread-safe? We run into the same problem as
  // EventWorkspaceMRU.
  // static Spectrum spec(*this, 0);
  // spec.m_index = i;
  // return spec;
  return Spectrum(*this, i);
}

void Workspace::push_back(const Spectrum &spectrum) {
  m_histograms.push_back(spectrum.histogram());
  m_spectrumNumbers.push_back(spectrum.spectrumNumber());
}

int main() {
  Workspace ws(4);
  std::vector<Spectrum> specs;
  specs.push_back(ws[3]);
  specs.push_back(ws[2]);
  auto spec = ws[3];
  Workspace out(0);
  for (const auto &spec : ws)
    printf("%d\n", spec.spectrumNumber());
  // std::remove_copy_if(ws.begin(), ws.end(), std::back_inserter(out),
  //                    [](const Spectrum &a) { return a.spectrumNumber() < 2;
  //                    });
}
*/

template <class A, class B> struct Item;

template <class A, class B> struct PairProxy {
  PairProxy(std::vector<A> &a, std::vector<B> &b) : a(a), b(b) {}

  // Note return by value but cannot be const (need ConstItem).
  Item<A, B> operator[](const size_t index);
  // ConstItem<A, B> operator[](const size_t index) const;

  std::vector<A> &a;
  std::vector<B> &b;
};

template <class A, class B> struct Item {
  Item(PairProxy<A, B> &container, const size_t index)
      : container(container), index(index) {}
  A &first() { return container.a[index]; }
  const A &first() const { return container.a[index]; }
  B &second() { return container.b[index]; }
  const B &second() const { return container.b[index]; }
  // Item(const Item &other) = default;
  // Note assignment of held values, not Item content.
  const Item &operator=(const Item &other) {
    first() = other.first();
    second() = other.second();
  }

  PairProxy<A, B> &container;
  size_t index;
};

template <class A, class B>
Item<A, B> PairProxy<A, B>::operator[](const size_t index) {
  return {*this, index};
}

// template <class A, class B>
// ConstItem<A, B> PairProxy<A, B>::operator[](const size_t index) const {
//   return {*this, index};
// }

template <class A, class B>
class Iterator
    : public boost::iterator_facade<Iterator<A, B>, const std::pair<A, B> &,
                                    boost::bidirectional_traversal_tag> {
public:
  Iterator(PairProxy<A, B> &proxy, const size_t index)
      : m_proxy(proxy), m_index(index){};

private:
  friend class boost::iterator_core_access;

  void increment() { ++m_index; }

  bool equal(const Iterator &other) const { return m_index == other.m_index; }

  const std::pair<A, B> &dereference() { return m_proxy; }

  void decrement() { --m_index; }

  void advance(int64_t delta) { m_index += delta; }

  uint64_t distance_to(const Iterator &other) const {
    return static_cast<uint64_t>(other.m_index) -
           static_cast<uint64_t>(m_index);
  }

  PairProxy<A, B> &m_proxy;
  size_t m_index;
};

#include <iostream>

// using namespace ranges;

int main() {
  std::vector<int> a1{15, 7, 3, 5};
  std::vector<int> a2{1, 2, 6, 21};
  auto view = ranges::view::zip(a1, a2);
  ranges::sort(view.begin(), view.end() - 1);
  for (const auto &item : view)
    printf("%d %d\n", item.first, item.second);

  std::vector<double> a{1, 2.5, 3.3};
  std::vector<int> b{1, 2, 3};
  PairProxy<double, int> proxy(a, b);
  proxy[2] = proxy[1];
  for (size_t i = 0; i < 3; ++i)
    printf("%lf %d\n", proxy[i].first(), proxy[i].second());

  // for (const auto &d : data)
  //   printf("%lf\n", d);
}
