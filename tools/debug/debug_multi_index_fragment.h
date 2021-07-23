/// Include in the MultiIndex class.

friend std::ostream &operator<<(std::ostream &os, const MultiIndex<N> &index) {
  os << "MultiIndex<" << N << "> {\n";
  os << "  data_index = " << index.m_data_index << '\n';
  os << "  stride = " << index.m_stride << '\n';
  os << "  coord = " << index.m_coord << '\n';
  os << "  shape = " << index.m_shape << '\n';
  os << "  ndim = " << index.m_ndim << '\n';
  os << "  stride = [";
  for (scipp::index dim = 0; dim < std::max(index.m_ndim, scipp::index{2});
       ++dim) {
    os << '[';
    for (scipp::index data = 0; data < N; ++data)
      os << index.m_stride[dim][data] << ' ';
    os << "] ";
  }
  os << "]\n";
  os << "  coord = [";
  for (scipp::index dim = 0; dim < std::max(index.m_ndim, scipp::index{2});
       ++dim)
    os << index.m_coord[dim] << ' ';
  os << "]\n";
  os << "  shape = [";
  for (scipp::index dim = 0; dim < std::max(index.m_ndim, scipp::index{2});
       ++dim)
    os << index.m_shape[dim] << ' ';
  os << "]\n";
  os << "  inner_ndim = " << index.m_inner_ndim << '\n';
  os << "  bin_stride = " << index.m_bin_stride << '\n';
  os << "  nested_dim_index = " << index.m_nested_dim_index << '\n';
  os << "  bin = [\n";
  for (auto &bin : index.m_bin) {
    os << "         bin_index = " << bin.m_bin_index << '\n';
  }
  os << "        ]\n";
  os << "}" << std::endl;
  return os;
}
