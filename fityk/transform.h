// This file is part of fityk program. Copyright 2001-2013 Marcin Wojdyr
// Licence: GNU General Public License ver. 2+
/// virtual machine for dataset transformations (@n = ...)

#ifndef FITYK_TRANSFORM_H_
#define FITYK_TRANSFORM_H_

#include "vm.h"

namespace fityk {

class Data;
class DataKeeper;

void run_data_transform(const DataKeeper& dk, const VMData& vm, Data* data_out);

} // namespace fityk
#endif // FITYK_TRANSFORM_H_
