include(scipp-util)

scipp_unary(math abs)
scipp_unary(math exp)
scipp_unary(math log)
scipp_unary(math log10)
scipp_unary(math reciprocal)
scipp_unary(math sqrt)

setup_scipp_category(math)

scipp_dataset_unary(trigonometry sin)
scipp_dataset_unary(trigonometry cos)
scipp_dataset_unary(trigonometry tan)
scipp_dataset_unary(trigonometry asin)
scipp_dataset_unary(trigonometry acos)
scipp_dataset_unary(trigonometry atan)

setup_scipp_category(trigonometry)

scipp_binary(comparison equal)
scipp_binary(comparison greater)
scipp_binary(comparison greater_equal)
scipp_binary(comparison less)
scipp_binary(comparison less_equal)
scipp_binary(comparison not_equal)

setup_scipp_category(comparison)
