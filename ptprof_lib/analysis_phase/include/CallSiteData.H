#ifndef CALL_SITE_DATA_H
#define CALL_SITE_DATA_H

#include <string>

struct CallSiteData {
  std::string cs_filename;
  int cs_line_number;
  bool par_for;
};

#endif
