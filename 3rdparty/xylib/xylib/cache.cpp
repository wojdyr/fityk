// Implementation of Public API of xylib library.
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: xylib.cpp 459 2009-04-15 23:14:50Z wojdyr $

#include "cache.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "xylib.h"
#include "util.h"

using namespace std;

namespace {

// Returns last modification time of the file or 0 if error occurs.
// There is a function boost::filesystem::last_write_time(), but it requires
// linking with Boost.Filesystem library and this would cause more problems
// than it's worth.
// Portable libraries such as wxWidgets and Boost.Filesystem get mtime using 
// ::GetFileTime() on MS Windows.
// Apparently some compilers also use _stat/_stat64 instead of stat.
// This will be implemented when portability problems are reported.
time_t get_file_mtime(string const& path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == -1) 
        return 0;
    return sb.st_mtime;
}

} // anonymous namespace

namespace xylib {

Cache* Cache::instance_ = NULL;

// not thread-safe
shared_ptr<const DataSet> Cache::load_file(string const& path, 
                                           string const& format_name,
                                           vector<string> const& options)
{
    vector<CachedFile>::iterator iter;
    for (iter = cache_.begin(); iter < cache_.end(); ++iter) {
        if (path == iter->path_ && format_name == iter->format_name_
                && options == iter->dataset_->options) {
#if 1
            time_t mtime = get_file_mtime(path);
            if (mtime != 0 && mtime < iter->read_time_)
#else
            // if we can't check mtime, keep cache for 2 seconds
            if (time(NULL) - 2 < iter->read_time_) 
#endif
                return iter->dataset_;
            else {
                cache_.erase(iter);
                break;
            }
        }
    }
    // this can throw exception
    shared_ptr<const DataSet> ds(xylib::load_file(path, format_name, options));

    if (cache_.size() >= n_cached_files_)
        cache_.erase(cache_.begin());
    cache_.push_back(CachedFile(path, format_name, ds));
    return ds;
}

void Cache::set_number_of_cached_files(size_t n)
{ 
    n_cached_files_ = n; 
    if (n > cache_.size())
        cache_.erase(cache_.begin() + n, cache_.end());
}

} // namespace xylib

