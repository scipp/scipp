Implementation of class Variable
================================

The implementation of ``class Variable`` and its views ``class VariableConstView`` and ``class VariableView`` are based on two interlinked requirements, type erasure and the concept of views supporting slicing.
The following figure gives an overview of the relations of the involved classes, with relevant details discussed below.
This is simplified to only display essential concepts, omitting many methods:

.. image:: ../../images/variable_classes.svg
  :width: 640
  :alt: Variable, related views, and type-erasure mechanism

Type erasure
------------

Type erasure is implemented using Sean Parent's *concept based polymorphism*.
The pattern is implemented using classes ``VariableConceptHandle``, ``VariableConcept``, and ``DataModel<T>``.

- ``VariableConceptHandle`` provides value-semantics for ``VariableConcept``, essentially providing a copy constructor based on ``VariableConcept::clone()``.
  ``VariableConcept`` is held in a ``std::unique_ptr`` since it is an abstract class.
- ``VariableConcept`` is an abstract class responsible for type erasure.
  Its interface contains pure-virtual methods providing common functionality such as copying (``clone()``), accessing the element type (``dtype()``), or comparison (``equals``).
- ``DataModel<T>`` is a templated child class of ``VariableConcept``.
  It implements the pure-virtual methods declared in ``VariableConcept`` and provides additional methods that depend on ``T``, i.e., for accessing the held arrays of values and variances.
  Note that this is the *same* template for all element types ``T`` (with the exception of binned data, see below), i.e., while there are many child classes of ``VariableConcept`` they are actually all instances of the same class template.

In addition to ``Variable``, which contains an array of values held by ``DataModel<T>``, we need to support *views* such as ``VariableConstView``.
The latter holds ``ElementArrayViewParams`` (or, in practice, members to construct such an instance).
These parameters are passed to ``DataModel<T>::values`` or ``DataModel<T>::variances`` to construct a typed ``ElementArrayView<<T>`` with the desired spatial slicing applied.
Interoperability between ``Variable`` and ``VariableConstView`` is based on ``ElementArrayView<<T>``.

Views and variables
-------------------

- Instances of ``Variable`` contain arrays of data.
  The member ``Variable::m_data`` of type ``VariableConceptHandle`` thus holds a ``DataModel<T>``.
- Instances of ``VariableConstView`` and ``VariableView`` contains views into arrays of data.

The views cannot simply make use of the constness of the instance, much like the iterators in the standard library come in ``const`` and non-``const``.
Therefore both the ``const`` pointer ``VariableConstView::m_variable`` and the non-``const`` pointer ``VariableView::m_mutableVariable`` are required, even though ``VariableView`` inherits ``VariableConstView``.
Access to values or variances via a view will use the respective pointer to access the underlying ``DataModel<T>`` of the variable.
This is where the ``const``-related overload resolution happens.

VariableConstView
-----------------

To support operations without requiring an abundance of overloads, read-only arguments are passed as ``VariableConstView``.
``Variable`` and ``VariableView`` then also work with these operations since:

- ``VariableView`` inherits ``VariableConstView``.
- ``Variable`` is implicitly convertible to ``VariableConstView``.

Binned variables
----------------

Binned variables are implemented using a specialization of ``DataModel<T>``.
There is an accompanying specialization of ``ElementArrayView<T>``.
