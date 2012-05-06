// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// virtual machine for dataset transformations (@n = ...)

#ifndef FITYK_TRANSFORM_H_
#define FITYK_TRANSFORM_H_

#include "vm.h"

namespace fityk {

class DatasetTransformer
{
public:
    DatasetTransformer(Ftk* F) : F_(F) {}
    void run_dt(const VMData& vm, int out);
private:
    Ftk* F_;
};

} // namespace fityk
#endif // FITYK_TRANSFORM_H_
