#pragma once

// @cond TYPE_ERASURE_DETAIL

#include <cassert>
#include <memory>
#include <typeinfo>
#include <type_traits>

namespace type_erasure_table_detail
{
    template < class T >
    inline void* clone_impl ( void* impl )
    {
        assert(impl);
        return new T( *static_cast<T*>( impl ) );
    }

    template < class T >
    inline void move_into_shared_ptr ( void* impl, std::shared_ptr<void>& ptr )
    {
        assert(impl);
        ptr = std::make_shared<T>( std::move(*static_cast<T*>( impl )) );
    }

    template < class T >
    inline void clone_into_shared_ptr ( void* impl, std::shared_ptr<void>& ptr )
    {
        assert(impl);
        ptr = std::make_shared<T>( *static_cast<T*>( impl ) );
    }

    template< class T, class Buffer >
    inline void* clone_into_buffer( void* impl, Buffer& buffer ) noexcept( std::is_nothrow_copy_constructible<T>::value )
    {
        assert(impl);
        new (&buffer) T( *static_cast<T*>( impl ) );
        return &buffer;
    }

    template< class T, class Buffer >
    inline void clone_into_buffer( void* impl, Buffer& buffer, std::shared_ptr<void>& impl_ ) noexcept( std::is_nothrow_copy_constructible<T>::value )
    {
        assert(impl);
        new (&buffer) T( *static_cast<T*>( impl ) );
	    impl_ = std::shared_ptr<T>( std::shared_ptr<T>( nullptr ), static_cast<T*>( static_cast<void*>( &buffer ) ) );
    }

    template< class T, class Buffer >
    inline void move_into_buffer( void* impl, Buffer& buffer, std::shared_ptr<void>& impl_ ) noexcept( std::is_nothrow_copy_constructible<T>::value )
    {
        assert(impl);
        new (&buffer) T( std::move(*static_cast<T*>( impl ) ) );
	    impl_ = std::shared_ptr<T>( std::shared_ptr<T>( nullptr ), static_cast<T*>( static_cast<void*>( &buffer ) ) );
    }

    template < class T >
    inline void delete_impl ( void* impl ) noexcept
    {
        assert(impl);
        delete static_cast<T*>( impl );
    }

    template < class T >
    inline T* cast_impl( void* impl ) noexcept
    {
        assert(impl);
        if( impl )
            return static_cast<T*>( impl );
        return nullptr;
    }

    template < class T >
    inline T* dynamic_cast_impl( const std::size_t& type_id, void* impl ) noexcept
    {
        assert(impl);
        if( impl && type_id == typeid( T ).hash_code() )
            return static_cast<T*>( impl );
        return nullptr;
    }

    template < class T >
    inline const T* cast_impl( const void* impl ) noexcept
    {
        assert(impl);
        if( impl )
            return static_cast<const T*>( impl );
        return nullptr;
    }


    inline char* char_ptr ( void* ptr ) noexcept
    {
        assert(ptr);
        return static_cast<char*>( ptr );
    }

    inline const char* char_ptr( const void* ptr ) noexcept
    {
        assert(ptr);
        return static_cast<const char*>( ptr );
    }

    template < class Buffer >
    inline std::ptrdiff_t impl_offset ( void* impl, Buffer& buffer ) noexcept
    {
        assert(impl);
        return char_ptr(impl) - char_ptr( static_cast<void*>(&buffer) );
    }

    template < class Buffer >
    inline bool is_heap_allocated ( void* impl, const Buffer& buffer ) noexcept
    {
        return impl < static_cast<const void*>(&buffer) ||
               static_cast<const void*>( char_ptr(&buffer) + sizeof(buffer) ) <= impl;
    }

    template < class Buffer >
    inline bool heap_allocated ( void* impl, const Buffer& buffer ) noexcept
    {
        return is_heap_allocated( impl, buffer );
    }

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
