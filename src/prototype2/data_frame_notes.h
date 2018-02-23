#ifndef DATA_FRAME_H
#define DATA_FRAME_H

#include <tuple>

// see https://stackoverflow.com/questions/48729384/is-there-still-a-reason-to-use-int-in-c-code
namespace gsl { using index = ptrdiff_t; }

template <class T> class CountsMixin {
  // problem if multiple columns of same type!
  const Counts &counts() const { return m_counts; }

private:
  Counts m_counts;
};

template <class T>
const std::string &name(const T &column) {
  return column.name();
}

// By having default name based on type we can avoid overhead for naming all
// components in all histograms. Wrap in class providing custom name in case
// required?
const std::string &name<Counts>(const Counts &counts) {
  static std::string name("Counts");
  return name;
}

// Workspace type will be different, is that an issue?
// issue with handling things via base class?
template <class T>
class Named : public T {
  public:
    const std::string &name() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }
  private:
    std::string m_name;
}

template <class Axis, class... Columns>
class DataFrame {
  public:
    gsl::index size() const { return std::get<0>(m_columns).size(); }
    // How to get columns? We what if we need multiple columns of same type? add
    // getters by name? (what if column does not have name?)

    template <class T> const T &get() const {
      return std::get<T>(m_columns);
    }
    template <class T> void set(const T &column) {
      std::get<T>(m_columns) = column;
    }

    const auto &column(const std::string &name) const; //??
    // do we need a column ID?
    // what does client code need to do:
    // - access specific field such as counts and bin edges -> dedicated accessor method, if no duplicates
    // - iterate all fields -> indexed access
    // - check existance/absence of specific field -> find(name)? (how to handle type-based names for duplicate types)?
    //   -> vector of type-erased columns?
    // should DataFrame have a name? Do we need objects without axis?
    // std::get<Counts> // C++14 :))

    class Item {

    };
    auto operator[](const size_t i) const {
      return std::make_tuple(m_columns[i]);
    }
  private:
    Axis m_axis;
    std::vector<string> m_columnNames; // leave this empty by default to reduce overhead in histograms?
    std::tuple<Columns...> m_columns;
};

template <class... Ts>
class Columns {
  private:
    std::tuple<Ts...> m_columns;
};

// YAGNI? Does it make sense to have a generic structure base on DataFrame, or
// should we just write all cases by hand?

class MultiCounts {
  private:
    std::string m_name; // single name shared among all spectra
    std::vector<Counts> m_counts; // Counts for all spectra
}; // would make Histogram not a DataFrame (just linking to MultiCounts)




#endif // DATA_FRAME_H
