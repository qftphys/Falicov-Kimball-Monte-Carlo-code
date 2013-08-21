#ifndef __FK_MC_COMMON_H
#define __FK_MC_COMMON_H

#include "definitions.hpp"

#include <triqs/arrays.hpp>
#include <triqs/arrays/algorithms.hpp>
#include <triqs/parameters/parameters.hpp>

#include <tuple>
#include <array>
#include <boost/math/special_functions/pow.hpp>
#include <numeric>

namespace fk {

namespace tqa = triqs::arrays;

using triqs::arrays::prod;
using triqs::arrays::sum;

#define MSG_PREFIX            __FILE__ << ":" << __LINE__ << ": "
#ifdef FK_MC_DEBUG
#define MY_DEBUG(MSG)            std::cout << std::boolalpha << MSG_PREFIX << MSG << std::endl; 
#else 
#define MY_DEBUG(MSG)
#endif
#define INFO(MSG)             std::cout << std::boolalpha << MSG << std::endl;
#define INFO2(MSG)            std::cout << "    " << std::boolalpha << MSG << std::endl;
#define INFO_NONEWLINE(MSG)   std::cout << MSG << std::flush;
#define ERROR(MSG)            std::cerr << MSG_PREFIX << MSG << std::endl;

typedef triqs::utility::parameters parameters;

template <size_t D>
inline std::ostream& operator<< (std::ostream& in, const std::array<size_t, D> arr){ 
    in << "{";  std::copy(arr.begin(), arr.end()-1, std::ostream_iterator<size_t>(in, ",")); in << *(arr.end()-1) << "}";
    return in;
};

/** A tool to split a tuple from http://stackoverflow.com/questions/10626856/how-to-split-a-tuple. */
template <typename Target, typename Tuple, int N, bool end >
struct __split_tuple_struct
{
    template < typename ... Args >
    static Target create(Tuple const& t, Args && ... args)
    {
        return __split_tuple_struct<Target,Tuple, N+1, std::tuple_size<Tuple>::value == N+1>::create(t, std::forward<Args>(args)..., std::get<N>(t));
    }
};

template < typename Target, typename Tuple, int N >
struct __split_tuple_struct<Target,Tuple,N,true>
{
    template < typename ... Args >
    static Target create(Tuple const& t, Args && ... args) { return Target(std::forward<Args>(args)...); }
};

template < typename Head, typename ... Tail >
inline std::tuple<Tail...> tuple_tail(std::tuple<Head,Tail...> const& tpl)
{
    return __split_tuple_struct<std::tuple<Tail...>, std::tuple<Head,Tail...>, 1, std::tuple_size<std::tuple<Head,Tail...>>::value == 1>::create(tpl);
}

}; // end of namespace FK

#endif // endif :: #ifndef __FK_MC_COMMON_H


