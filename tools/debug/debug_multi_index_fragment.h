/// Include in the MultiIndex class.


friend std::ostream &operator<<(std::ostream &os, const MultiIndex<N> &index) {
  os << "MultiIndex<" << N << "> {\n";
  os << "  data_index = " << index.m_data_index << '\n';
  os << "  ndim = " << index.m_ndim << '\n';
  os << "  stride = " << index.m_stride << '\n';
  os << "  coord = " << index.m_coord << '\n';
  os << "  shape = " << index.m_shape << '\n';
  os << "  end_sentinel = " << index.m_end_sentinel << '\n';
  os << "  ndim_nested = " << index.m_ndim_nested << '\n';
  os << "  nested_stride = " << index.m_nested_stride << '\n';
  os << "  nested_dim_index = " << index.m_nested_dim_index << '\n';
  os << "  bin = [\n";
  for (auto &bin : index.m_bin) {
    os << "         bin_index = " << bin.m_bin_index << '\n';
  }
  os << "           ]\n";
  os << "}" << std::endl;
  return os;
}
