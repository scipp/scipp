/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <numeric>
#include <set>

#include "range/v3/algorithm.hpp"
#include "range/v3/view/zip.hpp"

#include "dataset.h"

Dataset::Dataset(const Slice<const Dataset> &view) {
  for (const auto &var : view)
    insert(var);
}

Dataset::Dataset(const Slice<Dataset> &view) {
  for (const auto &var : view)
    insert(var);
}

Slice<const Dataset> Dataset::operator[](const std::string &name) const {
  return Slice<const Dataset>(*this, name);
}

Slice<Dataset> Dataset::operator[](const std::string &name) {
  return Slice<Dataset>(*this, name);
}

Slice<const Dataset> Dataset::operator()(const Dim dim, const gsl::index begin,
                                         const gsl::index end) const {
  return Slice<const Dataset>(*this)(dim, begin, end);
}

Slice<Dataset> Dataset::operator()(const Dim dim, const gsl::index begin,
                                   const gsl::index end) {
  return Slice<Dataset>(*this)(dim, begin, end);
}

void Dataset::insert(Variable variable) {
  if (variable.isCoord() && count(variable.type()))
    throw std::runtime_error("Attempt to insert duplicate coordinate.");
  if (!variable.isCoord()) {
    for (const auto &item : m_variables)
      if (item.type() == variable.type() && item.name() == variable.name())
        throw std::runtime_error(
            "Attempt to insert data of same type with duplicate name.");
  }
  // TODO special handling for special variables types like
  // Data::Histogram (either prevent adding, or extract into underlying
  // variables).
  mergeDimensions(variable.dimensions(), coordDimension[variable.type()]);
  m_variables.push_back(std::move(variable));
}

// T can be Dataset or Slice.
template <class T>
bool contains(const T &dataset, const Tag tag, const std::string &name) {
  for (gsl::index i = 0; i < dataset.size(); ++i)
    if (dataset[i].type() == tag.value() && dataset[i].name() == name)
      return true;
  return false;
}

bool Dataset::contains(const Tag tag, const std::string &name) const {
  return ::contains(*this, tag, name);
}

void Dataset::erase(const Tag tag, const std::string &name) {
  const auto it = m_variables.begin() + findUnique(tag);
  const auto dims = it->dimensions();
  m_variables.erase(it);
  for (const auto dim : dims.labels()) {
    bool found = false;
    for (const auto &var : m_variables)
      if (var.dimensions().contains(dim))
        found = true;
    if (!found)
      m_dimensions.erase(dim);
  }
}

Dataset Dataset::extract(const std::string &name) {
  Dataset subset;
  for (auto it = m_variables.begin(); it != m_variables.end();) {
    if (it->name() == name) {
      subset.insert(*it);
      it = m_variables.erase(it);
    } else {
      ++it;
    }
  }
  if (subset.size() == 0)
    throw std::runtime_error(
        "Dataset::extract(): No matching variable found in Dataset.");
  return subset;
}

gsl::index Dataset::find(const uint16_t id, const std::string &name) const {
  return ::find(*this, id, name);
}

gsl::index Dataset::count(const uint16_t id) const {
  gsl::index n = 0;
  for (auto &item : m_variables)
    if (item.type() == id)
      ++n;
  return n;
}

gsl::index Dataset::count(const uint16_t id, const std::string &name) const {
  gsl::index n = 0;
  for (auto &item : m_variables)
    if (item.type() == id && item.name() == name)
      ++n;
  return n;
}

gsl::index Dataset::findUnique(const Tag tag) const {
  gsl::index index = -1;
  for (gsl::index i = 0; i < size(); ++i) {
    if (m_variables[i].type() == tag.value()) {
      if (index != -1)
        throw std::runtime_error(
            "Given variable tag is not unique. Must provide a name.");
      index = i;
    }
  }
  if (index == -1)
    throw std::runtime_error("Dataset does not contain such a variable.");
  return index;
}

void Dataset::mergeDimensions(const Dimensions &dims, const Dim coordDim) {
  gsl::index found = 0;
  for (gsl::index i = 0; i < dims.count(); ++i) {
    const auto dim = dims.label(i);
    auto size = dims.size(i);
    bool found = false;
    for (gsl::index j = 0; j < m_dimensions.count(); ++j) {
      if (m_dimensions.label(j) == dim) {
        if (m_dimensions.size(j) == size) {
          found = true;
          break;
        }
        // coordDim is `Dim::Invalid` if there is no coordinate dimension.
        if (dim == coordDim) {
          if (m_dimensions.size(j) == size - 1) {
            // This is an edge coordinate, merge reduced dimension.
            --size;
            found = true;
            break;
          }
          throw std::runtime_error(
              "Cannot insert variable into Dataset: Variable is a dimension "
              "coordiante, but the dimension length matches neither as default "
              "coordinate nor as edge coordinate.");
        } else {
          if (m_dimensions.size(j) == size + 1) {
            // If the dataset so far contains only edge variables for this
            // dimension, shrink its size.
            bool canShrink = true;
            for (const auto &var : m_variables) {
              if (var.dimensions().contains(dim) &&
                  coordDimension[var.type()] != dim)
                canShrink = false;
            }
            if (canShrink) {
              m_dimensions.resize(dim, size);
              found = true;
              break;
            }
          }
          throw std::runtime_error(
              "Cannot insert variable into Dataset: Dimensions do not match.");
        }
      }
    }
    // TODO Add after checking all so we can give strong exception guarantee.
    if (!found) {
      m_dimensions.add(dim, size);
    }
  }
}

bool Dataset::operator==(const Dataset &other) const {
  return (m_dimensions == other.m_dimensions) &&
         (m_variables == other.m_variables);
}

Dataset &Dataset::operator+=(const Dataset &other) {
  for (const auto &var2 : other.m_variables) {
    // Handling of missing variables:
    // - Skip if this contains more (automatic by having enclosing loop over
    //   other instead of *this).
    // - Fail if other contains more.
    gsl::index index;
    try {
      index = find(var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Right-hand-side in addition contains variable "
                               "that is not present in left-hand-side.");
    }
    auto &var1 = m_variables[index];
    if (var1.isCoord()) {
      // Coordinate variables must match
      // Strictly speaking we should allow "equivalent" coordinates, i.e., match
      // only after projecting out any constant dimensions.
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot perform addition");
      // TODO We could improve sharing here magically, but whether this is
      // beneficial would depend on the shared reference count in var1 and var2:
      // var1 = var2;
    } else if (var1.isData()) {
      if (var1.valueTypeIs<Data::History>()) {
        auto hists = var1.get<Data::History>();
        if (hists.size() != 1)
          throw std::runtime_error("TODO: History should be 0-dimensions. "
                                   "Flatten it? Prevent creation? Do we need "
                                   "history with dimensions?");
        const auto &hist2 = var2.get<const Data::History>()[0];
        const gsl::index size = hist2.size();
        for (gsl::index i = 0; i < size; ++i)
          hists[0].push_back("other." + hist2[i]);
        hists[0].push_back("operator+=");
      } else {
        // Data variables are added
        var1 += var2;
      }
    } else {
      // Attribute variables are added
      // TODO Does it make sense to do this only if mismatched?
      if (var1 != var2)
        var1 += var2;
    }
  }
  return *this;
}

template <class T1, class T2> T1 &minus_equals(T1 &dataset, const T2 &other) {
  std::set<std::string> names;
  for (const auto &var2 : other)
    if (var2.isData())
      names.insert(var2.name());

  for (const auto &var2 : other) {
    gsl::index index;
    try {
      index = find(dataset, var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      if (var2.isData() && names.size() == 1) {
        // Only a single (named) variable in RHS, subtract from all.
        index = -1;
      } else {
        throw std::runtime_error("Right-hand-side in subtraction contains "
                                 "variable that is not present in "
                                 "left-hand-side.");
      }
    }
    using VarRef = std::conditional_t<std::is_same<T1, Dataset>::value,
                                      Variable &, VariableSlice<Variable>>;
    if (index >= 0) {
      VarRef var1 = detail::makeAccess(dataset)[index];
      if (var1.isCoord()) {
        if (!(var1 == var2))
          throw std::runtime_error(
              "Coordinates of datasets do not match. Cannot "
              "perform subtraction.");
      } else if (var1.isData()) {
        if (var1.type() == tag_id<Data::Variance>)
          var1 += var2;
        else
          var1 -= var2;
      }
    } else {
      // Not a coordinate, subtract from all.
      gsl::index count = 0;
      for (VarRef var1 : detail::makeAccess(dataset)) {
        if (var1.type() == var2.type()) {
          ++count;
          if (var1.type() == tag_id<Data::Variance>)
            var1 += var2;
          else
            var1 -= var2;
          // TODO Cannot change name from slice, what should we do here?
          if (!var1.data().isView())
            var1.setName(var1.name() + " - " + var2.name());
        }
      }
      if (count == 0)
        throw std::runtime_error("Right-hand-side in subtraction contains "
                                 "variable type that is not present in "
                                 "left-hand-side.");
    }
  }
  return dataset;
}

template <class T> Dataset &Dataset::operator-=(const T &other) {
  return minus_equals(*this, other);
}

template <class D>
bool Slice<D>::contains(const Tag tag, const std::string &name) const {
  return ::contains(*this, tag, name);
}

template bool Slice<Dataset>::contains(const Tag, const std::string &) const;
template bool Slice<const Dataset>::contains(const Tag,
                                             const std::string &) const;

template <class T>
Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::operator-=(const T &other) {
  return minus_equals(base(), other);
}

VariableSlice<Variable>
SliceMutableMixin<Slice<Dataset>>::get(const gsl::index i) {
  return detail::makeSlice(
      detail::makeAccess(base().m_dataset)[base().m_indices[i]],
      base().m_slices);
}

dataset_slice_iterator<Dataset>
SliceMutableMixin<Slice<Dataset>>::mutableBegin() const {
  return {base().m_dataset, base().m_indices, base().m_slices, 0};
}
dataset_slice_iterator<Dataset>
SliceMutableMixin<Slice<Dataset>>::mutableEnd() const {
  return {base().m_dataset, base().m_indices, base().m_slices,
          static_cast<gsl::index>(base().m_indices.size())};
}
const Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::base() const {
  return static_cast<const Slice<Dataset> &>(*this);
}
Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::base() {
  return static_cast<Slice<Dataset> &>(*this);
}

template Dataset &Dataset::operator-=(const Dataset &);
template Dataset &Dataset::operator-=(const Slice<const Dataset> &);
template Dataset &Dataset::operator-=(const Slice<Dataset> &);

template Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::
operator-=(const Dataset &);
template Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::
operator-=(const Slice<const Dataset> &);
template Slice<Dataset> &SliceMutableMixin<Slice<Dataset>>::
operator-=(const Slice<Dataset> &);

namespace aligned {
// Helpers to define a pointer to aligned memory.
// alignas cannot be used like this, e.g., clang rejects it. Need to find
// another way.
// template <class T> using type alignas(32) = T;
template <class T> using type = T;
template <class T> using ptr = type<T> *;

// Using restrict does not seem to help much?
#define RESTRICT __restrict
void multiply(const gsl::index size, ptr<double> RESTRICT v1,
              ptr<double> RESTRICT e1, ptr<const double> RESTRICT v2,
              ptr<const double> RESTRICT e2) {
  for (gsl::index i = 0; i < size; ++i) {
    e1[i] = e1[i] * (v2[i] * v2[i]) + e2[i] * (v1[i] * v1[i]);
    v1[i] *= v2[i];
  }
}
} // namespace aligned

Dataset &Dataset::operator*=(const Dataset &other) {
  // See operator+= for additional comments.
  for (const auto &var2 : other.m_variables) {
    gsl::index index;
    try {
      index = find(var2.type(), var2.name());
    } catch (const std::runtime_error &) {
      throw std::runtime_error("Right-hand-side in addition contains variable "
                               "that is not present in left-hand-side.");
    }
    if (var2.type() == tag_id<Data::Variance>) {
      try {
        find(tag_id<Data::Value>, var2.name());
        other.find(tag_id<Data::Value>, var2.name());
      } catch (const std::runtime_error &) {
        throw std::runtime_error("Cannot multiply datasets that contain a "
                                 "variance but no corresponding value.");
      }
    }
    auto &var1 = m_variables[index];
    if (var1.isCoord()) {
      // Coordinate variables must match
      if (!(var1 == var2))
        throw std::runtime_error(
            "Coordinates of datasets do not match. Cannot perform addition");
    } else if (var1.isData()) {
      // Data variables are added
      if (var2.type() == tag_id<Data::Value>) {
        if (count(tag_id<Data::Variance>, var2.name()) !=
            other.count(tag_id<Data::Variance>, var2.name()))
          throw std::runtime_error("Either both or none of the operands must "
                                   "have a variance for their values.");
        if (count(tag_id<Data::Variance>, var2.name()) != 0) {
          auto error_index1 = find(tag_id<Data::Variance>, var2.name());
          auto error_index2 = other.find(tag_id<Data::Variance>, var2.name());
          auto &error1 = m_variables[error_index1];
          auto &error2 = other.m_variables[error_index2];
          if ((var1.dimensions() == var2.dimensions()) &&
              (var1.dimensions() == error1.dimensions()) &&
              (var1.dimensions() == error2.dimensions())) {
            // Optimization if all dimensions match, avoiding allocation of
            // temporaries and redundant streaming from memory of large array.
            error1.setUnit(var2.unit() * var2.unit() * error1.unit() +
                           var1.unit() * var1.unit() * error2.unit());
            var1.setUnit(var1.unit() * var2.unit());

            auto v1 = var1.get<Data::Value>();
            auto v2 = var2.get<const Data::Value>();
            auto e1 = error1.get<Data::Value>();
            auto e2 = error2.get<const Data::Value>();
            aligned::multiply(v1.size(), v1.data(), e1.data(), v2.data(),
                              e2.data());
          } else {
            error1 = error1 * (var2 * var2) + var1 * var1 * error2;
            // TODO: Catch errors from unit propagation here and give a better
            // error message.
            var1 *= var2;
          }
        } else {
          // No variance found, continue without.
          var1 *= var2;
        }
      } else if (var2.type() == tag_id<Data::Variance>) {
        // Do nothing, math for variance is done when processing corresponding
        // value.
      } else {
        var1 *= var2;
      }
    }
  }
  return *this;
}

void Dataset::setSlice(const Dataset &slice, const Dimension dim,
                       const gsl::index index) {
  for (const auto &var2 : slice.m_variables) {
    auto &var1 = m_variables[find(var2.type(), var2.name())];
    if (var1.dimensions().contains(dim))
      var1.setSlice(var2, dim, index);
    else
      var1 = var2;
  }
  if (count(tag_id<Data::History>) == 1)
    get<Data::History>()[0].push_back("this->setSlice(slice, dim, " +
                                      std::to_string(index) + ");");
}

template <class Value>
VariableSlice<
    std::conditional_t<std::is_const<Value>::value, const Variable, Variable>>
dataset_slice_iterator<Value>::dereference() const {
  return detail::makeSlice(detail::makeAccess(m_dataset)[m_indices[m_index]],
                           m_slices);
}

template class dataset_slice_iterator<Dataset>;
template class dataset_slice_iterator<const Dataset>;

Dataset operator+(Dataset a, const Dataset &b) { return a += b; }
Dataset operator-(Dataset a, const Dataset &b) { return a -= b; }
Dataset operator*(Dataset a, const Dataset &b) { return a *= b; }

Dataset slice(const Dataset &d, const Dimension dim, const gsl::index index) {
  // TODO It is up for debate whether this should always throw if the dimension
  // is not contained or only with non-zero index.
  if (!d.dimensions().contains(dim) && index != 0)
    throw std::runtime_error("Slice index out of range");
  Dataset out;
  for (const auto &var : d) {
    if (var.dimensions().contains(dim))
      out.insert(slice(var, dim, index));
    else
      out.insert(var);
  }
  try {
    out.get<Data::History>()[0].push_back("slice(., dim, " +
                                          std::to_string(index) + ");");
  } catch (std::runtime_error &) { // no history
  }
  return out;
}

Dataset slice(const Dataset &d, const Dimension dim, const gsl::index begin,
              const gsl::index end) {
  if (!d.dimensions().contains(dim) && (begin != 0 || end != 1))
    throw std::runtime_error("Slice index out of range");
  Dataset out;
  for (const auto &var : d) {
    if (var.dimensions().contains(dim))
      out.insert(slice(var, dim, begin, end));
    else
      out.insert(var);
  }
  return out;
}

std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices) {
  std::vector<Dataset> out(indices.size() + 1);
  for (const auto &var : d) {
    if (var.dimensions().contains(dim)) {
      auto vars = split(var, dim, indices);
      for (gsl::index i = 0; i < out.size(); ++i)
        out[i].insert(vars[i]);
    } else {
      for (auto &o : out)
        o.insert(var);
    }
  }
  return out;
}

Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dim dim) {
  // Match type and name, drop missing?
  // What do we have to do to check and compute the resulting dimensions?
  // - If dim is in m_dimensions, *some* of the variables contain it. Those that
  //   do not must then be identical (do not concatenate) or we could
  //   automatically broadcast? Yes!?
  // - If dim is new, concatenate variables if different, copy if same.
  // We will be doing deep comparisons here, it would be nice if we could setup
  // sharing, but d1 and d2 are const, is there a way...? Not without breaking
  // thread safety? Could cache cow_ptr for future sharing setup, done by next
  // non-const op?
  Dataset out;
  for (gsl::index i1 = 0; i1 < d1.size(); ++i1) {
    const auto &var1 = d1[i1];
    const auto &var2 = d2[d2.find(var1.type(), var1.name())];
    // TODO may need to extend things along constant dimensions to match shapes!
    if (var1.dimensions().contains(dim)) {
      out.insert(concatenate(var1, var2, dim));
    } else {
      if (var1 == var2) {
        out.insert(var1);
      } else {
        if (d1.dimensions().contains(dim)) {
          // Variable does not contain dimension but Dataset does, i.e.,
          // Variable is constant. We need to extend it before concatenating.
          throw std::runtime_error("TODO");
        } else {
          // Creating a new dimension
          out.insert(concatenate(var1, var2, dim));
        }
      }
    }
  }
  return out;
}

Dataset convert(const Dataset &d, const Dimension from, const Dimension to) {
  // How to convert? There are several cases:
  // 1. Tof conversion as Mantid's ConvertUnits.
  // 2. Axis conversion as Mantid's ConvertSpectrumAxis.
  // 3. Conversion of multiple dimensions simultaneuously, e.g., to Q, which
  //    cannot be done here since it affects more than one input and output
  //    dimension. Should we have a variant that accepts a list of dimensions
  //    for input and output?
  // 4. Conversion from 1 to N or N to 1, e.g., Dim::Spectrum to X and Y pixel
  //    index.
  if (!d.dimensions().contains(from))
    throw std::runtime_error(
        "Dataset does not contain the dimension requested for conversion.");
  // Can Dim::Spectrum be converted to anything? Should we require a matching
  // coordinate when doing a conversion? This does not make sense:
  // auto converted = convert(dataset, Dim::Spectrum, Dim::Tof);
  // This does if we can lookup the TwoTheta, make axis here, or require it?
  // Should it do the reordering? Is sorting separately much less efficient?
  // Dim::Spectrum is discrete, Dim::TwoTheta is in principle contiguous. How to
  // handle that? Do we simply want to sort instead? Discrete->contiguous can be
  // handled by binning? Or is Dim::TwoTheta implicitly also discrete?
  // auto converted = convert(dataset, Dim::Spectrum, Dim::TwoTheta);
  // This is a *derived* coordinate, no need to store it explicitly? May even be
  // prevented?
  // DatasetView<const Coord::TwoTheta>(dataset);
}

Dataset rebin(const Dataset &d, const Variable &newCoord) {
  Dataset out;
  if (!newCoord.isCoord())
    throw std::runtime_error(
        "The provided rebin coordinate is not a coordinate variable.");
  const auto dim = coordDimension[newCoord.type()];
  if (dim == Dim::Invalid)
    throw std::runtime_error(
        "The provided rebin coordinate is not a dimension coordinate.");
  const auto &newDims = newCoord.dimensions();
  if (!newDims.contains(dim))
    throw std::runtime_error("The provided rebin coordinate lacks the "
                             "dimension corresponding to the coordinate.");
  if (!isContinuous(dim))
    throw std::runtime_error(
        "The provided rebin coordinate is not a continuous coordinate.");
  const auto &oldCoord = d[d.findUnique(Tag(newCoord.type()))];
  const auto &oldDims = oldCoord.dimensions();
  const auto &datasetDims = d.dimensions();
  if (!oldDims.contains(dim))
    throw std::runtime_error("Existing coordinate to be rebined lacks the "
                             "dimension corresponding to the new coordinate.");
  if (oldDims.size(dim) != datasetDims.size(dim) + 1)
    throw std::runtime_error("Existing coordinate to be rebinned is not a bin "
                             "edge coordinate. Use `resample` instead of rebin "
                             "or convert to histogram data first.");
  for (gsl::index i = 0; i < newDims.ndim(); ++i) {
    const auto newDim = newDims.label(i);
    if (newDim == dim)
      continue;
    if (datasetDims.contains(newDim)) {
      if (datasetDims.size(newDim) != newDims.shape()[i])
        throw std::runtime_error(
            "Size mismatch in auxiliary dimension of new coordinate.");
    }
  }
  // TODO check that input as well as output coordinate are sorted in rebin
  // dimension.
  for (const auto &var : d) {
    if (!var.dimensions().contains(dim)) {
      out.insert(var);
    } else if (var.type() == newCoord.type()) {
      out.insert(newCoord);
    } else {
      out.insert(rebin(var, oldCoord, newCoord));
    }
  }
  return out;
}

// We can specialize this to switch to a more efficient variant when sorting
// datasets that represent events lists, using LinearView.
template <class Tag> Dataset sort(const Dataset &d, const std::string &name) {
  auto const_axis = d.get<const Tag>(name);
  if (d.dimensions<Tag>(name).count() != 1)
    throw std::runtime_error("Axis for sorting must be 1-dimensional.");
  const auto sortDim = d.dimensions<Tag>().label(0);
  if (const_axis.size() != d.dimensions().size(sortDim))
    throw std::runtime_error("Axis for sorting cannot be a bin-edge axis.");
  if (std::is_sorted(const_axis.begin(), const_axis.end()))
    return d;

  Dataset sorted;
  auto axisVar = d[d.find(tag_id<Tag>, name)];
  auto axis = axisVar.template get<Tag>();
  std::vector<gsl::index> indices(axis.size());
  std::iota(indices.begin(), indices.end(), 0);
  auto view = ranges::view::zip(axis, indices);
  using ranges::sort;
  sort(view.begin(), view.end(), [](const auto &a, const auto &b) {
    return std::get<0>(a) < std::get<0>(b);
  });
  // Joint code for all tags, extract into function to reduce instantiated code
  // size?
  for (const auto &var : d) {
    if (!var.dimensions().contains(sortDim))
      sorted.insert(var);
    else if (var.type() == tag_id<Tag> && var.name() == name)
      sorted.insert(axisVar);
    else
      sorted.insert(permute(var, sortDim, indices));
  }
  return sorted;
}

#define CASE_RETURN(TAG, FUNC, ...)                                            \
  case tag<TAG>.value():                                                       \
    return FUNC<TAG>(__VA_ARGS__);

Dataset sort(const Dataset &d, Tag t, const std::string &name) {
  switch (t.value()) {
    CASE_RETURN(Coord::X, sort, d, name);
    CASE_RETURN(Data::Value, sort, d, name);
  default:
    throw std::runtime_error(
        "Sorting by this variable type has not been implemented.");
  }
}

Dataset filter(const Dataset &d, const Variable &select) {
  if (select.dimensions().ndim() != 1)
    throw std::runtime_error(
        "Cannot filter variable: The filter must by 1-dimensional.");
  const auto dim = select.dimensions().labels()[0];

  Dataset filtered;
  for (auto &var : d)
    if (var.dimensions().contains(dim))
      filtered.insert(filter(var, select));
    else
      filtered.insert(var);
  return filtered;
}
