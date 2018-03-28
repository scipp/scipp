#ifndef DATA_FRAME_H
#define DATA_FRAME_H

#include <string>
#include <tuple>

// see https://stackoverflow.com/questions/48729384/is-there-still-a-reason-to-use-int-in-c-code
namespace gsl { using index = ptrdiff_t; }

template <class T>
const std::string &name(const T &column) {
  return column.name();
}

// By having default name based on type we can avoid overhead for naming all
// components in all histograms. Wrap in class providing custom name in case
// required?
//const std::string &name<Counts>(const Counts &counts) {
//  static std::string name("Counts");
//  return name;
//}

template <class Axis, class... Columns>
class DataFrame {
  public:
    DataFrame() = default;
    DataFrame(Axis &&axis, const std::vector<std::string> &columnNames,
              Columns &&... columns)
        : m_axis(std::forward<Axis>(axis)), m_columnNames(columnNames),
          m_columns(std::forward<Columns>(columns)...) {
      if (columnNames.size() != std::tuple_size<std::tuple<Columns...>>::value)
        throw std::runtime_error("");
    }
    DataFrame(Axis &&axis, Columns &&... columns)
        : m_axis(std::forward<Axis>(axis)),
          m_columns(std::forward<Columns>(columns)...) {
      // TODO check length of axis, depending on whether it is BinEdges or
      // Points
    }

    gsl::index size() const { return std::get<0>(m_columns).size(); }

    template <class T> const T &get() const {
      return std::get<T>(m_columns);
    }
    template <class T> void set(const T &column) {
      std::get<T>(m_columns) = column;
    }

    // const auto &column(const std::string &name) const; //every column has a different type, not possible.

    auto operator[](const size_t i) const {
      return std::make_tuple(m_columns[i]);
    }

  private:
    Axis m_axis;
    std::vector<std::string> m_columnNames; // leave this empty by default to reduce overhead in histograms?
    std::tuple<Columns...> m_columns;
};

// Important aspects:
// - Encode unit in type of BinEdges, Counts, ...

// TODO
// - Axis with shape (multi dimensional)
// - recursive processing, merging, ...
// - should Axis be read-only? Can we support generators etc.?

#endif // DATA_FRAME_H
