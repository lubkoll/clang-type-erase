#pragma once

// @cond TYPE_ERASURE_DETAIL

#include <cassert>
#include <memory>
#include <typeinfo>
#include <type_traits>

namespace type_erasure_table_detail
{
    template < class T >
    using voider = void;

    template < class T >
    struct remove_reference_wrapper
    {
        using type = T;
    };

    template < class T >
    struct remove_reference_wrapper< std::reference_wrapper< T > >
    {
        using type = T;
    };

    template < class T >
    using remove_reference_wrapper_t = typename remove_reference_wrapper< T >::type;

    template < class... Args >
    struct AndImpl;

    template <class Arg, class... Args>
    struct AndImpl<Arg, Args...>
    {
        using type = std::integral_constant<bool, Arg::value && AndImpl<Args...>::type::value>;
    };

    template <class Arg>
    struct AndImpl<Arg>
    {
        using type = Arg;
    };

    template <class... Args>
    using And = typename AndImpl<Args...>::type;
}

// @endcond
