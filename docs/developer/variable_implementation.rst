Implementation of class Variable
================================

The implementation of ``class Variable`` and its views ``class VariableConstView`` and ``class VariableView`` are based on two interlinked requirements, type erasure and the concept of views supporting slicing.
The following figure gives an overview of the relations of the involved classes, with relevant details discussed below.
This is simplified to only display essential concepts, omitting many methods:

.. image:: ../images/variable_classes.svg
  :width: 640
  :alt: Variable, related views, and type-erasure mechanism

Type erasure
------------

Type erasure is implemented using Sean Parent's *concept based polymorphism*.
If we only had to deal with ``Variable`` the pattern would be implemented using classes ``VariableConceptHandle``, ``VariableConcept``, and ``DataModel<T>``.
For simplicity, let us initially ignore the classes ``VariableConceptT<T>`` and ``ViewModel<T>``, imagining that ``DataModel<T>`` is a direct child of ``VariableConcept``:

- ``VariableConceptHandle`` provides value-semantics for ``VariableConcept``, essentially providing a copy constructor based on ``VariableConcept::clone()``.
  ``VariableConcept`` is held in a ``std::unique_ptr`` since it is an abstract class.
- ``VariableConcept`` is an abstract class responsible for type erasure.
  Its interface contains pure-virtual methods providing common functionality such as copying (``clone()``), accessing the element type (``dtype()``), or comparison (``operator==``).
- ``DataModel<T>`` is a templated child class of ``VariableConcept``.
  It implements the pure-virtual methods declared in ``VariableConcept`` and provides additional methods that depend on ``T``, i.e., for accessing the held arrays of values and variances.
  Note that this is the *same* template for all element types ``T``, i.e., while there are many child classes of ``VariableConcept`` they are actually all instances of the same class template.

In addition to ``Variable``, which contains an array of values held by ``DataModel<T>``, we need to support *views* such as ``VariableConstView``.
This is provided by ``ViewModel<T>``, which also implements the interface of ``VariableConcept``.
However, to support *interoperability* between ``Variable`` and ``VariableConstView`` an additional level of inheritance is added to the original pattern.
Essentially, we require a common interface between ``DataModel<T>`` an ``ViewModel<T>`` such that operations do not need to support all possible permutations of the two but can instead be implemented using the interface.

- The common interface is provided by ``VariableConceptT<T>``.
- ``VariableConceptT<T>`` implements some of (but not all of) the interface of ``VariableConcept``.
- ``VariableConceptT<T>`` additionally extends the interface with the ``values()`` and ``variances()`` methods which do depend on ``T`` and were thus not defined as part of the type-erased interface given by ``VariableConcept``.

Views and variables
-------------------

- Instances of ``Variable`` contain arrays of data.
  The member ``Variable::m_data`` of type ``VariableConceptHandle`` thus holds a ``DataModel<T>``.
- Instances of ``VariableConstView`` and ``VariableView`` contains views into arrays of data.
  The member ``VariableConstView::m_view`` of type ``VariableConceptHandle`` thus holds a ``ViewModel<T>``.

Unfortunately, there is an additional complication required for handling/distinguishing ``const`` and non-``const`` access.
The views cannot simply make use of the constness of the instance, much like the iterators in the standard library come in ``const`` and non-``const``.
Naively this would lead to, e.g., distinguishing ``VariableConceptT<double>`` and ``VariableConceptT<const double>``, but this would result in a combinatoric explosion of the number of cases to handle and implement in operations with multiple inputs.

- We solve this by not handling constness as part of the type of ``VariableConceptT<T>``.
- Instead, ``ViewModel<double>`` and ``ViewModel<const double>`` *both* implement ``VariableConceptT<double>`` .
- Attempts to call mutating methods of ``VariableConceptT<T>`` will then lead to a *runtime* failure.
  Note that in practice this can never happen since this is wrapped in the higher-level view classes.

VariableConstView
-----------------

To support operations without requiring an abundance of overloads, read-only arguments are passed as ``VariableConstView``.
``Variable`` and ``VariableView`` then also work with these operations since:

- ``VariableView`` inherits ``VariableConstView``.
- ``Variable`` is implicitly convertible to ``VariableConstView``.
