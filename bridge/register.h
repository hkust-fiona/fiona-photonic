#ifndef FIONA_REGISTER
#define FIONA_REGISTER

#include <utility>
#include <vector>
#include <string>

typedef std::pair<std::string, std::string> PyFileFunc;
typedef std::vector<PyFileFunc> PyFileFuncVec;

const PyFileFuncVec pyfilefunc_reg {
    {"test", "ops_single"},
    {"test", "ops_dual"},
    {"ideal_numerical", "dotp"},
    {"ideal_numerical", "mvm"}
};

#endif /* FIONA_REGISTER */
