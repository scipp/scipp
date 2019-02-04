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

On second thought though, this probably still suffers from the combinatoric explosion:
The type of the converting view unfortunately also depends on the type it is converting from, so we only moved the problem to another place.


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
// 6. Views of named variables.
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

As a counter argument, consider that for variables on their own it never actually makes a difference whether they represent coordinates, data, variances, other something else.
The distinction becomes meaningful only once included in a dataset --- at which point we could support another mechanism to distinguish those concepts.
It would however become more tricky and dangerous to move that auxiliary information from one dataset to another, consider

```cpp
// If tag is part of Variable:
dataset.insert(variable);
// If tags (or a similar concept) exist only on the Dataset level:
dataset.insert(Coord::X, variable); // Prone bugs due to typos in tags.
```


### Concepts

```
Number
String
Table
Vector (in space, i.e., not std::vector but, e.g., Eigen::Vector3d)
Matrix
NumberList
ComplexNumber?
Continuous/Noncontinuous?

Coordinates?
CoordX = Number && Continuous
CoordY = Number && Continuous # what is the difference to X?
SpectrumNumber = Number # Integer?

Dimension-coordinates?
-> need concept of dimension, which is for the variable, not the element type!
```

https://github.com/ldionne/dyno


`Variable` holds a `VariableConcept`.
The latter is a bit too generic and requires manually disabling functionality for types that do not support certain operations.
Can we adapt the mechanism?
`VariableConcept` is generic, anything that can be stored in a variable should be able to fulfil the interface.
Can we introduce another level to get to a more concrete/refined concept?
 
```
VariableConcept
NumberVariableConcept # can apply binary operations
NumberVariableT # ops for concrete type defined here
DataModel<T> / ViewModel<T> # defines data layout, but not specific to concept?
DataModel<T, Concept> # some implementation, but inherit specific concept to enable class of operations
```

How to get from VariableConcept to NumberVariableConcept:

```cpp
Variable operator+=(const Variable &other) {
  // This does not work, since data() returns VariableConcept, which would not have +=.
  data() += other.data();
  // Instead:
  dynamic_cast<NumberVariableConcept &>(data()) += other.data();
  // Wrap it nicely:
  require<NumberVariable>() += other.data();
}

Variable rebin(const Variable &var, const Variable &oldCoord,
               const Variable &newCoord) {
  return require<NumberVariable>().rebin(var.require<NumberVariable>(),
                                         oldCoord.require<ContinuousCoord>(),
                                         newCoord.require<ContinuousCoord>());
}
```

Data::Tofs, Data::PulseTimes, Data::Weights, Data::Variances are all "List"? But we want the same name!

Operations that combine two different concepts? Matrix-vector multiplication?

- Always specifying the data type is inconvenient in practice, consider, e.g.,

  ```cpp
  auto ids = d.get<boost::small_vector<int32_t, 1>>(Coord::DetectorGrouping);
  // or
  auto ids = d.get<DetectorGrouping>(Coord::DetectorGrouping);
  ```

  In practice, client code does not care about the actualy type, and the type is essentially always the same.
  It would therefore be more convenient if it could be omitted:

  ```cpp
  // Coord::DetectorGrouping is now a constexpr instance and can be used to deduce
  //  the return type. Note that we lose the ability to specify constness.
  auto ids = d.get(Coord::DetectorGrouping);
  ```

- Do not support data-tags or attribute-tags that are not for "grouping" purposes!
  - Should we drop `Data::Value`?
    Otherwise untagged variables will not match data with uncertainties.
    ```cpp
    // Getting array access works (assuming untagged also implies data type).
    auto data = dataset.get("sample1"); // default is double
    auto vars = dataset.get(Data::Variance, "sample1");
    auto strings = dataset.get<std::string>(Data::Value, "comment"); // custom type
    // Variable view?
    // This would return also the variance, how to get only data?
    auto data = dataset("sample1");
    // Maybe just make getting a subset more explicit:
    DatasetSlice subset = dataset.subset("sample1"); // return coords and all vars with that name
    VariableSlice data = dataset("sample1");
    gsl::span<std:string> s = data.get<std::string>();
    VariableSlice vars = dataset(Data::Value, "sample1");
    VariableSlice vars = dataset(Data::Variance, "sample1");

    DatasetSlice vars = dataset("sample1");
    auto data = vars.get<double>(); // works if only a single variable is contained in vars
    VariableSlice vars = dataset("sample1")(Data::Value);
    VariableSlice vars = dataset("sample1")(Data::Variance);

    dataset.subset("sample") -= dataset.subset("background");
    // Dangerous trap: ignores variances! (currently syntax includes them)
    dataset("sample") -= dataset("background");
    ```
  - Dropping `Data::Value` does not seem to work?
    Should `Data::Value` be the default tag?

- Should support data and attributes without tag!?
  Also non-dimension coordinates without tag?

  ```cpp
  // If we support tag-less variables, should this still return a DatasetSlice?
  // Yes, why not, it is then basically like an xarray.DataArray.
  auto var = dataset("sample1");

  // Using default type:
  auto x = dataset.get(Coord::X);
  // Custom type if default is not used:
  auto x = dataset.get<float>(Coord::X);
  // Tag-less variable:
  auto data = dataset.get<double>("sample1");
  // Tagged and named:
  // Can use default type
  auto data = dataset.get(Data::Value, "sample1");
  // Or custom
  auto data = dataset.get<float>(Data::Value, "sample1");

  // Accessing Variable:
  auto data = var.get<float>();
  // Globally default to double?
  auto data = var.get();
  // Use default type (and check tag at same time):
  auto data = var.get(Coord::X);
  // Use custom type (and check tag at same time):
  auto data = var.get<float>(Coord::X);
  ```


#### Conclusion

- At the `Variable` level:
  - The tag has no more meaning than the name.
    This is reinforced by what we see in the current implementation:
    The `VariableConcept` member of `Variable` (which is used to implement all the operations) does not know the tag.
  - The item type alone defines which operations can be applied to a variable.
- At the `Dataset` level:
  - Tags define a relationship between variables.
    Tags come in groups, and operations are defined for specific groups.
    - Example 1: A data value and a variance form a group (a value with uncertainty).
    - Example 2: A list of time-of-flight values and (optionally) lists of pulse-times, weights, and uncertainties form a group (an event-list).
  - Names of variables define a "family" of variables.
    Only variables from the same family are considered related when looking for variables with tags from the same tag group.
- If we think in terms of concepts, which in turn define which operations can be applied:
  - On the `Variable` level, the item type can fulfil specific concepts.
  - On the `Dataset` level, the concrete tag group can fulfil specific concepts.

  Another way to think about this is in terms of array-of-structures (AOS) instead of structure-of-arrays (SOA):
  - For AOS the type fully defines which operations can be applied to it.
    Everything could then be done on the `Variable`-lebel.
  - `Dataset` is actually using SOA in many cases.
    Therefore some of the concepts are not fully definable for `Variable`, i.e., we need a mechanism for this on the `Dataset` level.
    A definition of tag groups (alongside implementation of operations) provides such a mechanism.

A rough outline of how this could be implemented on the `Dataset` level, with custom handlers for custom groups of tags:

```cpp
/// in dataset.cpp

class VariableGroup {
public:
  virtual void operator*=(const VariableGroup &other) = 0;

protected:
  // Probably need also a ConstVariableGroup
  std::vector<Variable &> m_vars;
};

class ValueWithError : public VariableGroup {
public:
  // Somehow define which tags are in the group
  // static auto tags() { return {Data::Value, Data::Variance}; }
  ValueWithError(const std::vector<Variable &> vars) : VariableGroup(vars) {
    assert(m_vars[0].tag() == Data::Value);
    assert(m_vars[1].tag() == Data::Variance);
  }

  void operator*=(const VariableGroup &other) override {
    m_vars[1] *= other.m_vars[0] * other.m_vars[0];
    m_vars[1] += m_vars[0] * m_vars[0] * other.m_vars[1];
    m_vars[0] *= other.m_vars[0];
  }
};

Dataset &Dataset::operator*=(const Dataset &other) {
  for (const auto &var : other) {
    // May be done already by being pulled in as group member from other tag
    if (done(var))
      continue;
    // List of all tags in the group
    const auto tagGroup = var.tag().group();
    // Get complete groups of variables.
    const auto otherVarGroup = other.getGroup(tagGroup);
    auto varGroup = getGroup(tagGroup);
    // Virtual call to the implementation for the dynamic group type.
    varGroup *= otherVarGroup; // May throw if this group does not support the
                               // operation.
  }
}
```
