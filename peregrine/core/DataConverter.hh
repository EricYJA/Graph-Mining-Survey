#ifndef DATA_CONVERTER_HH
#define DATA_CONVERTER_HH

#include <string>

namespace Peregrine
{
  namespace DataConverter
  {
    void convert_data(const std::string &edge_file,
        const std::string &label_file,
        const std::string &out_dir, 
        uint16_t label_flat_size = 1, 
        uint16_t label_flat_space = 1);
  }
}

#endif
