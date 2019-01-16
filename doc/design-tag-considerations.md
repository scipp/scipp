## Tags, names, and grouping variables in Dataset

See also a [recent PR](https://github.com/mantidproject/dataset/pull/8) which was the final trigger to reconsider the tag mechanism.


### Current solution

1. Names of variables define a grouping, which is used, e.g., to associate a variance with its data.
2. Tags map directly to types.
   This happens at compile time, i.e., a specific tag always defines a type.
3. Tags are types (not `enum` values) such that they can be `const`-qualified.
   This is necessary for the copy-on-write mechanism.
   Runtime-tags are also supporting using a base class that holds an ID.

The combination of defining a type and `const` qualification supports the following syntax:

```cpp
// 1. Creating variables and type-erased access.
dataset.insert<Data::Value>("sample1", {});
dataset.insert<Data::Value>("sample2", {}, {0.1});
auto dataView = dataset(Data::Value{}, "sample1");

// 2. Read-only access to a single variable.
auto data = dataset.get<const Data::Value>();

// 3. Write access to a single variable.
auto data = dataset.get<Data::Value>();

// 4. Associating variables with each other.
auto data = dataset.get<const Data::Value>("sample1");
auto errors = dataset.get<const Data::Variance>("sample1");

// 5. Typed view of multiple variables.
DatasetView<const Coord::X, Data::Value> view(dataset);

// 6. Views of named variable.
DatasetView<const Coord::X, Data::Value> view(dataset, "sample1");

// 7. Getters with compile-time tags.
auto &value = view.begin()->get<Data::Value>();

// 8. Named getters.
auto &x = view.begin()->x();

// 9. View of multiple named variables.
// Currently not supported since it requires an unclear mapping from template
// arguments to name strings and, more importantly, it would require getters
// with string arguments and comparisons, which would be very inefficient at
// this level.
DatasetView<const Coord::X, Data::Value, const Data::Value>
    view(dataset, "sample1", "sample2");
// Note that we avoid a positional way of resolving this: The order of arguments
// is not reflected in the type of the view.
auto &value = view.begin()->get<Data::Value>("sample1");
```


The examples are numbered and we will discuss alternative below.


### Runtime tags, flexible type

```cpp
// 1. Creating variables and type-erased access.
// If no data is given, the type must be specified explicitly.
dataset.insert<double>(Data::Value, "sample1", {});
// Variable type is deduced.
dataset.insert(Data::Value, "sample1", {}, {0.1});
auto dataView = dataset(Data::Value, "sample1");

// 2. Read-only access to a single variable.
auto data = dataset.get<const double>(Data::Value);

// 3. Write access to a single variable.
auto data = dataset.get<double>(Data::Value);

// 4. Associating variables with each other.
auto data = dataset.get<const double>(Data::Value, "sample1");
auto errors = dataset.get<const double>(Data::Variance, "sample1");

// 5. Typed view of multiple variables.
// This is awkward since we must mentally match a template argument to each tag.
DatasetView<const double, double> view(dataset, Coord::X, Data::Value);
// Clear but slightly more verbose.
auto view = dataset.makeView(Label<const double, Coord::X>{},
                             Label<double, Data::Value>{});

// 6. Views of named variables.
auto view = dataset.makeView(Label<const double, Coord::X>{},
                             Label<double, Data::Value>("sample1"));

// 7. Getters with compile-time tags.
// It may at first seem like this is not possible since the tag does not imply a
// type. However, the mapping from tag to type is fixed in the type of the view!
auto &value = view.begin()->get<Data::Value>();

// 8. Named getters.
auto &x = view.begin()->x();

// 9. View of multiple named variables.
// This solves the problem of mapping tags to names, but the item-getter issue remains.
auto view = dataset.makeView(Label<const double, Coord::X>{},
                             Label<double, Data::Value>("sample1"),
                             Label<const double, Data::Value>("sample2"));
auto &value = view.begin()->get<Data::Value>("sample1");
// Could provide a getter ID:
auto view =
    dataset.makeView(Label<const double, Coord::X>{},
                     Label<double, Data::Value, Label::Id1>("sample1"),
                     Label<const double, Data::Value, Label::Id2>("sample2"));
auto &value = view.begin()->get<Label::Id1>();
```

The distinction between types and tags raises the question whether this is sufficient.
Generic operations like addition would simply operate on all variables (or all variables that are addable).
For more specific operations like generating a histogram from event data this is not so clear, and there are at least two options:
- We can histogram any variable that has a compatible item type such as `std::vector<double>`.
- Or we can histogram only variables with specific predefined tags, such as `Data::Events`.

It feels like the first option is too zealous whereas the latter is too restrictive.
Do we thus need another conceptional level?
Currently we have `type < tag < name`, i.e., the tag defines a concept that is more specific than a type.
Would it make sense to extend this to `type < concept < tag < name`?
Probably not, we should rather make the type more specific, i.e., think of it as `concept < tag < name`?

#### Dealing with flexible types

In practice flexible types leads to a couple of things that need to be taken into consideration:

- Client code may be written for a fixed type, e.g., assume that everything is holding values of type `double`.
  This is perfectly fine in non-generic code.
  It will simply lead to runtime failure if a dataset with other types is passed.
- More generic code should support a range of types, e.g., `float` as well as `double`.
  This is relatively simple to do.
  Code is just templated on the type, and a call helper will generate the required runtime-to-compile time branching, see the [similar example](https://github.com/mantidproject/dataset/blob/d0957e4bfd87646010728656a9eb7512310238b1/src/dataset.cpp#L638) for the actually more complex case of supporting compile-time tags.

The main limitation of this is that for operations with multiple involved variables we will either restrict this to have the same type in all variables, or deal with a combinatoric explosion of cases:
Consider, e.g., the addition of two objects with associated errors where each can be stored in either single-precision or double-precision.
This would lead to 2x2x2x2 = 16 cases.
If also coordinates with flexible types are involved we quickly reach O(100) cases, which is unmanageable.
Therefore, in practice we will need to put certain limitations on the type combinations that are supported, even for the more generic operations.

One way to avoid combinatoric explosion may be to introduce a view that can convert precision on the fly.
For example, when adding a variable containing `double` items to a variable containing `float` items, the former could be converted to a `float` view.

```cpp
// Assume we want to do a computation in double precision.
template <class T>
double compute(const T &view) {
  double init = 0.0;
  return std::accumulate(view.begin(), view.end(), init);
}

// Not actually including more than one input in this example, so there is no
// "explosion", but the technique stays the same for multiple arguments.
if (var.type() != Type::Double) {
  auto doubleView = var.getConvertingView<float, double>();
  return compute(doubleView);
} else {
  auto doubleSpan = var.get<double>();
  return compute(doubleSpan);
}
```


#### Non-trivial and derived item types

If the type is defined by the tag, we can also hide implementation complexity.
For example, `Data::Events` could actually return something like `EventListProxy<Data::Tof, Data::PulseTime>`.
We can probably simply use a `typedef` to keep this convenience,

```cpp
template <class T>
using EventList = EventListProxy<Label<T, Data::Tof>, Label<int64_t, Data::PulseTime>>;
auto eventLists = dataset.get<EventList<double>>(Data::Events);
```

Another option is to specify only the precision in the getter, if we define a tag "category" at compile time and leave only the precision flexible:

```cpp
// Defined at compile time: Data::Events can only contain Data::Tof and
// Data::PulseTime, but precision is determined at runtime.
auto eventLists = dataset.get<double>(Data::Events);
```


### No tags, just names and types

```cpp
// 1. Creating variables and type-erased access.
// If no data is given, the type must be specified explicitly.
dataset.insert<double>("sample1", {});
// Variable type is deduced.
dataset.insert("sample1", {}, {0.1});
auto dataView = dataset("sample1");

// 2. Read-only access to a single variable.
auto data = dataset.get<const double>("sample1");

// 3. Write access to a single variable.
auto data = dataset.get<double>("sample1");

// 4. Associating variables with each other.
// Requires mechanism using string prefix/suffix and restrictions on names.
auto data = dataset.get<const double>("sample1");
auto errors = dataset.get<const double>("sample1-variance");

// 5. Typed view of multiple variables.
// 5. Views of named variables.
auto view =
    dataset.makeView(Label<const double>("Coord.X"), Label<double>("sample1"));

// 7. Getters with compile-time tags.
// Not possible unless we assign IDs when creating the view.

// 8. Named getters.
// Not possible unless we assign IDs when creating the view.

// 9. View of multiple named variables.
// See above.
```

### Other

It [has been criticised](https://github.com/mantidproject/dataset/pull/4#issuecomment-445320309) that the two-part item getters are awkward.
In place of `dataset[Data.Variance, 'sample1']` an alternative `dataset.variances['sample1']` was suggested, which follows what `xarray` does for coordinates.

Based on this example, this looks like a good suggestion, putting variances on a similar footing as coordinates.
However, there would be two drawbacks to this:

- We not only have data and variance that need to be grouped.
  Another key example is event data, which can include time-of-flight, pulse-times, and weights.
  If we did not have a grouping mechanism it would restrict our options when devising an event-storage format.
- Unless we drop the connection between tag and type, we need a tag on the C++ side.

Also note that we support storing data and variances in separate variables.
If the variance variable did not have a tag it would be impossible to tell what it is.
