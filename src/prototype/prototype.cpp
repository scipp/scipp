#include <cstdint>
#include "metadata.h"
#include "data.h"
#include "instrument.h"

template <class Data> class VectorWorkspace {
private:
  std::vector<Data> m_data;
  std::vector<SpectrumDefinition> m_spectrumDefinitions;
  std::vector<int32_t> m_spectrumNumbers;
  SpectrumInfo m_spectrumInfo;
  Logs m_logs;
};

int main() {}
