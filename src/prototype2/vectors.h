#ifndef VECTORS_H
#define VECTORS_H

// Ultimately we should probably not handle this via inheritance, but it should
// do for the prototype.
template <class Unit> class BinEdges : public std::vector<double> {
  using std::vector<double>::vector;
};
template <class Unit> class Points : public std::vector<double> {
  using std::vector<double>::vector;
};
template <class... Unit> class Counts : public std::vector<double> {
  using std::vector<double>::vector;
};
template <class... Unit> class CountStdDevs : public std::vector<double> {
  using std::vector<double>::vector;
};

// Counts<> -> normal counts
// Counts<Unit::PerSecond> etc.

#endif // VECTORS_H
