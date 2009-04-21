// Public API of xylib library.
// Licence: Lesser GNU Public License 2.1 (LGPL) 
// $Id: xylib.h 462 2009-04-15 23:54:27Z wojdyr $

/// This header is new in 0.4 and may be changed in future.
/// Support for caching files read by xylib.
/// Usage is similar to load_file() from xylib.h:
///     DataSet const& my_dataset = xylib::Cache::Get()->load_file(...);

#ifndef XYLIB_CACHE_H_
#define XYLIB_CACHE_H_

#ifndef __cplusplus
#error "This library does not have C API."
#endif

#include <ctime>
#include <string>
#include <vector>
#include <tr1/memory>

using std::tr1::shared_ptr;

namespace xylib
{


class DataSet;

// singleton
class Cache
{
public:
    // get instance
    static Cache* Get() 
        { if (instance_ == NULL) instance_ = new Cache(); return instance_; }

    // Arguments are the same as in load_file() in xylib.h, 
    // but a const ref is returned instead of pointer.
    DataSet const& load_file(std::string const& path, 
                             std::string const& format_name="",
                             std::vector<std::string> const& options 
                                                = std::vector<std::string>());

    // set max. number of cached files, default=1
    void set_number_of_cached_files(size_t n);
    // get max. number of cached files
    inline size_t get_number_of_cached_files() const { return n_cached_files_; }

    // clear cache
    void clear_cache() { cache_.clear(); }

private:
    static Cache *instance_; // for singleton pattern

    struct CachedFile
    {
        std::string path_;
        std::string format_name_;
        std::time_t read_time_;
        shared_ptr<const DataSet> dataset_;

        CachedFile(std::string const& path, 
                   std::string const& format_name, 
                   DataSet *dataset) 
            : path_(path), format_name_(format_name), 
              read_time_(std::time(NULL)), dataset_(dataset) {}
    };

    size_t n_cached_files_;
    std::vector<CachedFile> cache_;

    Cache() : n_cached_files_(1) {}
};

Cache* Cache::instance_ = NULL;

} // namespace xylib

#endif // XYLIB_CACHE_H_
