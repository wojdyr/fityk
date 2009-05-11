// This file is part of fityk program. Copyright (C) Marcin Wojdyr
// $Id$
// Licence of this file: either GPL or Boost Software License, Version 1.0.

#ifndef FITYK__OPTIONAL_SUFFIX__H
#define FITYK__OPTIONAL_SUFFIX__H

#include <boost/ref.hpp>
#include <boost/spirit/core/assert.hpp>
#include <boost/spirit/core/parser.hpp>
#include <boost/spirit/core/composite/impl/directives.ipp>

using namespace boost::spirit;

template<
    typename RT,
    typename IteratorT,
    typename ScannerT>
inline RT
optional_suffix_parser_parse(
    IteratorT base_first,
    IteratorT base_last,
    IteratorT suffix_first,
    IteratorT suffix_last,
    ScannerT& scan)
{
    typedef typename ScannerT::iterator_t iterator_t;
    iterator_t saved = scan.first;
    std::size_t slen = base_last - base_first;

    while (base_first != base_last)
    {
        if (scan.at_end() || (*base_first != *scan))
            return scan.no_match();
        ++base_first;
        ++scan;
    }

    while (suffix_first != suffix_last)
    {
        if (scan.at_end() || (*suffix_first != *scan))
            break;
        ++suffix_first;
        ++scan;
        ++slen;
    }

    return scan.create_match(slen, nil_t(), saved, scan.first);
}


///////////////////////////////////////////////////////////////////////////
//
//  optional_suffix_seq_parser class
//
///////////////////////////////////////////////////////////////////////////
template <typename IteratorT = char const*>
class optional_suffix_seq_parser :
         public parser<optional_suffix_seq_parser<IteratorT> >
{
public:

    typedef optional_suffix_seq_parser<IteratorT> self_t;

    optional_suffix_seq_parser(IteratorT base_first_, IteratorT base_last_,
                            IteratorT suffix_first_, IteratorT suffix_last_)
    : base_first(base_first_), base_last(base_last_),
      suffix_first(suffix_first_), suffix_last(suffix_last_) {}

    optional_suffix_seq_parser(IteratorT base_first_,
                               IteratorT suffix_first_)
    : base_first(base_first_),
      base_last(boost::spirit::impl::get_last(base_first_)),
      suffix_first(suffix_first_),
      suffix_last(boost::spirit::impl::get_last(suffix_first_)){}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename boost::unwrap_reference<IteratorT>::type striter_t;
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        return optional_suffix_parser_parse<result_t>(
            striter_t(base_first),
            striter_t(base_last),
            striter_t(suffix_first),
            striter_t(suffix_last),
            scan);
    }

private:

    IteratorT base_first;
    IteratorT base_last;
    IteratorT suffix_first;
    IteratorT suffix_last;
};


///////////////////////////////////////////////////////////////////////////
//
//  optional_suffix_parser class
//
///////////////////////////////////////////////////////////////////////////
template <typename IteratorT = char const*>
class optional_suffix_parser :
            public parser<optional_suffix_parser<IteratorT> >
{
public:

    typedef optional_suffix_parser<IteratorT> self_t;

    optional_suffix_parser(IteratorT base_first, IteratorT base_last,
                           IteratorT suffix_first, IteratorT suffix_last)
    : seq(base_first, base_last, suffix_first, suffix_last) {}

    optional_suffix_parser(IteratorT base_first, IteratorT suffix_first)
    : seq(base_first, suffix_first) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        return boost::spirit::impl::contiguous_parser_parse<result_t>
            (seq, scan, scan);
    }

private:

    optional_suffix_seq_parser<IteratorT> seq;
};

template <typename CharT>
inline optional_suffix_parser<CharT const*>
optional_suffix_p(CharT const* base_str, CharT const* suffix_str)
{
    return optional_suffix_parser<CharT const*>(base_str, suffix_str);
}

template <typename IteratorT>
inline optional_suffix_parser<IteratorT>
optional_suffix_p(IteratorT base_first, IteratorT base_last,
                  IteratorT suffix_first, IteratorT suffix_last)
{
    return optional_suffix_parser<IteratorT>(base_first, base_last,
                                             suffix_first, suffix_last);
}


#endif
