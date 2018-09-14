#pragma once

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>

namespace clang
{
    namespace type_erasure
    {
        namespace detail
        {
            template <class T>
            void deleteData(void* data) noexcept
            {
                assert(data);
                delete static_cast<T*>(data);
            }

            template <class T>
            void destructData(void* data) noexcept
            {
                assert(data);
                static_cast<T*>(data)->~T();
            }

            template <class T>
            void* copyData(void* data)
            {
                return data ? new T( *static_cast<T*>(data) ) : nullptr;
            }

            template <class T>
            std::shared_ptr<void> copyData(const std::shared_ptr<void>& data)
            {
                return data ? std::make_shared<T>(*static_cast<T*>(data.get())) : nullptr;
            }

            template< class T, class Buffer >
            void* copyIntoBuffer( void* data, Buffer& buffer ) noexcept( std::is_nothrow_copy_constructible<T>::value )
            {
                assert(data);
                new (&buffer) T( *static_cast<T*>( data ) );
                return &buffer;
            }

            template< class T, class Buffer >
            std::shared_ptr<void> copyIntoBuffer(const std::shared_ptr<void>& data, Buffer& buffer) noexcept( std::is_nothrow_copy_constructible<T>::value )
            {
                assert(data);
                new (&buffer) T( *static_cast<T*>( data.get() ) );
                return std::shared_ptr<T>( std::shared_ptr<T>(), static_cast<T*>( static_cast<void*>( &buffer ) ) );
            }

            inline const char* charPtr( const void* ptr ) noexcept
            {
                assert(ptr);
                return static_cast<const char*>( ptr );
            }

            template < class Buffer >
            bool isHeapAllocated (void* data, const Buffer& buffer) noexcept
            {
                // treat nullptr as heap-allocated
                if(!data)
                    return true;
                return data < static_cast<const void*>(&buffer) ||
                        static_cast<const void*>( charPtr(&buffer) + sizeof(buffer) ) <= data;
            }

            template <class T>
            struct IsReferenceWrapper : std::false_type
            {};

            template <class T>
            struct IsReferenceWrapper< std::reference_wrapper<T> > : std::true_type
            {};


            template < class T >
            struct RemoveReferenceWrapper
            {
                using type = T;
            };

            template < class T >
            struct RemoveReferenceWrapper< std::reference_wrapper< T > >
            {
                using type = T;
            };

            template < class T >
            using RemoveReferenceWrapper_t = typename RemoveReferenceWrapper< T >::type;
        }


        template <class Derived, bool rttiEnabled>
        class Casts
        {
        public:
            constexpr Casts(std::size_t typeId = 0) noexcept
                : id(typeId)
            {}

            template < class T >
            T* target() noexcept
            {
                auto data = static_cast<Derived*>(this)->write( );
                assert(data);
                if( data && id == typeid( T ).hash_code() )
                    return static_cast<T*>( data );
                return nullptr;
            }

            template < class T >
            const T* target( ) const noexcept
            {
                auto data = static_cast<const Derived*>(this)->read( );
                assert(data);
                if( data && id == typeid( T ).hash_code() )
                    return static_cast<const T*>( data );
                return nullptr;
            }

            template <class T>
            static constexpr Casts create() noexcept
            {
                return Casts(typeid(std::decay_t<T>).hash_code());
            }

        private:
            std::size_t id;
        };


        template <class Derived>
        class Casts<Derived, false>
        {
        public:
            template < class T >
            T* target() noexcept
            {
                auto data = static_cast<Derived*>(this)->write( );
                assert(data);
                return static_cast<T*>( static_cast<Derived*>(this)->write( ) );
            }

            template < class T >
            const T* target( ) const noexcept
            {
                auto data = static_cast<const Derived*>(this)->read( );
                assert(data);
                return static_cast<const T*>( data );
            }

            template <class T>
            static constexpr Casts create() noexcept
            {
                return Casts();
            }
        };


        template <class Derived, bool rttiEnabled>
        class Accessor : public Casts<Derived, rttiEnabled>
        {
        public:
            using Base = Casts<Derived, rttiEnabled>;

            constexpr Accessor( ) = default;

            explicit Accessor(Base base, bool containsReferenceWrapper) noexcept
                : Base(base)
                , containsReferenceWrapper(containsReferenceWrapper)
            {}

            template <class T>
            T& get() noexcept
            {
                auto data = static_cast<Derived*>(this)->write( );
                assert(data);
                if(containsReferenceWrapper)
                    return static_cast<std::reference_wrapper<T>*>(data)->get();
                return *static_cast<T*>(data);
            }

            template <class T>
            const T& get() const noexcept
            {
                const auto data = static_cast<const Derived*>(this)->read( );
                assert(data);
                if(containsReferenceWrapper)
                    return static_cast<const std::reference_wrapper<T>*>(data)->get();
                return *static_cast<const T*>(data);
            }

            explicit operator bool() const noexcept
            {
                return static_cast<const Derived*>(this)->read( ) != nullptr;
            }

            template <class T>
            static constexpr Accessor create(bool containsReferenceWrapper) noexcept
            {
                return Accessor(Base::template create<T>(), containsReferenceWrapper);
            }

        private:
            bool containsReferenceWrapper = false;
        };


        template<bool rttiEnabled>
        class Storage : public Accessor<Storage<rttiEnabled>, rttiEnabled>
        {
            friend class Accessor<Storage, rttiEnabled>;
            friend class Casts<Storage, rttiEnabled>;

            using Base = Accessor<Storage, rttiEnabled>;
        public:
            constexpr Storage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<Storage, std::decay_t<T> >::value>* = nullptr>
            explicit Storage(T&& value)
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  del(&detail::deleteData< std::decay_t<T> >),
                  copy_data(&detail::copyData< std::decay_t<T> >),
                  data(new std::decay_t<T>(std::forward<T>(value)))
            {}

            template <class T,
                      std::enable_if_t<!std::is_base_of<Storage, std::decay_t<T> >::value>* = nullptr>
            Storage& operator=(T&& value)
            {
                return *this = Storage(std::forward<T>(value));
            }

            ~Storage()
            {
                reset();
            }

            Storage(const Storage& other)
                : Base(other),
                  del(other.del),
                  copy_data(other.copy_data),
                  data(other.data == nullptr ? nullptr : other.copy())
            {}

            Storage(Storage&& other) noexcept
                : Base(other),
                  del(other.del),
                  copy_data(other.copy_data),
                  data(other.data)
            {
                other.data = nullptr;
            }

            Storage& operator=(const Storage& other)
            {
                reset();
                Base::operator=(other);
                del = other.del;
                copy_data = other.copy_data;
                data = (other.data == nullptr ? nullptr : other.copy());
                return *this;
            }

            Storage& operator=(Storage&& other) noexcept
            {
                reset();
                Base::operator=(other);
                del = other.del;
                copy_data = other.copy_data;
                data = other.data;
                other.data = nullptr;
                return *this;
            }

        private:
            void reset() noexcept
            {
                if(data)
                    del(data);
            }

            void* read() const noexcept
            {
                return data;
            }

            void* write() noexcept
            {
                return read();
            }

            void* copy() const
            {
                assert(data);
                return copy_data(data);
            }

            using delete_fn = void(*)(void*);
            using copy_fn = void*(*)(void*);
            delete_fn del = nullptr;
            copy_fn copy_data = nullptr;
            void* data = nullptr;
        };


        template<bool rttiEnabled>
        class NonCopyableStorage : public Accessor<NonCopyableStorage<rttiEnabled>, rttiEnabled>
        {
            friend class Accessor<NonCopyableStorage, rttiEnabled>;
            friend class Casts<NonCopyableStorage, rttiEnabled>;

            using Base = Accessor<NonCopyableStorage, rttiEnabled>;
        public:
            constexpr NonCopyableStorage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<NonCopyableStorage, std::decay_t<T> >::value>* = nullptr>
            explicit NonCopyableStorage(T&& value)
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  del(&detail::deleteData< std::decay_t<T> >),
                  data(new std::decay_t<T>(std::forward<T>(value)))
            {}

            template <class T,
                      std::enable_if_t<!std::is_base_of<NonCopyableStorage, std::decay_t<T> >::value>* = nullptr>
            NonCopyableStorage& operator=(T&& value)
            {
                return *this = NonCopyableStorage(std::forward<T>(value));
            }

            ~NonCopyableStorage()
            {
                reset();
            }

            NonCopyableStorage(NonCopyableStorage&& other) noexcept
                : Base(other),
                  del(other.del),
                  data(other.data)
            {
                other.data = nullptr;
            }

            NonCopyableStorage& operator=(NonCopyableStorage&& other) noexcept
            {
                reset();
                Base::operator=(other);
                del = other.del;
                data = other.data;
                other.data = nullptr;
                return *this;
            }

        private:
            void reset() noexcept
            {
                if(data)
                    del(data);
            }

            void* read() const noexcept
            {
                return data;
            }

            void* write() noexcept
            {
                return read();
            }

            using delete_fn = void(*)(void*);
            delete_fn del = nullptr;
            void* data = nullptr;
        };


        template<bool rttiEnabled>
        class COWStorage : public Accessor<COWStorage<rttiEnabled>, rttiEnabled>
        {
            friend class Accessor<COWStorage, rttiEnabled>;
            friend class Casts<COWStorage, rttiEnabled>;

            using Base = Accessor<COWStorage, rttiEnabled>;
        public:
            constexpr COWStorage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<COWStorage, std::decay_t<T> >::value>* = nullptr>
            explicit COWStorage(T&& value)
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  copy_data(&detail::copyData< std::decay_t<T> >),
                  data(std::make_shared< std::decay_t<T> >(std::forward<T>(value)))
            {}

            template <class T,
                      std::enable_if_t<!std::is_base_of<COWStorage, std::decay_t<T> >::value>* = nullptr>
            COWStorage& operator=(T&& value)
            {
                return *this = COWStorage(std::forward<T>(value));
            }

        private:
            void* read() const noexcept
            {
                return data.get();
            }

            void* write()
            {
                if(!data.unique())
                    data = copy_data(data);
                return read();
            }

            using copy_fn = std::shared_ptr<void>(*)(const std::shared_ptr<void>&);
            copy_fn copy_data = nullptr;
            std::shared_ptr<void> data = nullptr;
        };


        template <int buffer_size, bool rttiEnabled>
        class SBOStorage : public Accessor< SBOStorage<buffer_size, rttiEnabled>, rttiEnabled >
        {
            using Buffer = std::array<char,buffer_size>;

            struct FunctionTable
            {
                using delete_fn = void(*)(void*);
                using destruct_fn = void(*)(void*);
                using copy_fn = void*(*)(void*);
                using buffer_copy_fn = void*(*)(void*, Buffer&);

                delete_fn del = nullptr;
                destruct_fn destruct = nullptr;
                copy_fn copy = nullptr;
                buffer_copy_fn copy_into = nullptr;
            };

            friend class Accessor< SBOStorage, rttiEnabled >;
            friend class Casts<SBOStorage, rttiEnabled>;

            using Base = Accessor<SBOStorage, rttiEnabled>;
        public:
            constexpr SBOStorage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                      std::enable_if_t<std::greater<>()(sizeof(std::decay_t<T>), sizeof(Buffer))>* = nullptr>
            explicit SBOStorage(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  function_table{&detail::deleteData< std::decay_t<T> >,
                                 &detail::destructData< std::decay_t<T> >,
                                 &detail::copyData< std::decay_t<T> >,
                                 &detail::copyIntoBuffer<std::decay_t<T>, Buffer>},
                  data(new std::decay_t<T>(std::forward<T>(value)))
            {}

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                      std::enable_if_t<std::less_equal<>()(sizeof(std::decay_t<T>), sizeof(Buffer))>* = nullptr>
            explicit SBOStorage(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  function_table{&detail::deleteData< std::decay_t<T> >,
                                 &detail::destructData< std::decay_t<T> >,
                                 &detail::copyData< std::decay_t<T> >,
                                 &detail::copyIntoBuffer<std::decay_t<T>, Buffer>}
            {
                new(&buffer) std::decay_t<T>(std::forward<T>(value));
                data = &buffer;
            }

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr>
            SBOStorage& operator=(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
            {
                return *this = SBOStorage(std::forward<T>(value));
            }

            ~SBOStorage()
            {
                reset();
            }

            SBOStorage(const SBOStorage& other)
                : Base(other),
                  function_table(other.function_table)
            {
                data = other.copy_into(buffer);
            }

            SBOStorage(SBOStorage&& other) noexcept
                : Base(other),
                  function_table(other.function_table)
            {
                if(!other.data)
                    return;

                if(detail::isHeapAllocated(other.data, other.buffer))
                    data = other.data;
                else
                {
                    buffer = other.buffer;
                    data = &buffer;
                }

                other.data = nullptr;
            }

            SBOStorage& operator=(const SBOStorage& other)
            {
                reset();
                Base::operator=(other);
                function_table = other.function_table;
                data = other.copy_into(buffer);
                return *this;
            }

            SBOStorage& operator=(SBOStorage&& other) noexcept
            {
                reset();
                if(!other.data)
                {
                    data = nullptr;
                    return *this;
                }
                Base::operator=(other);
                function_table = other.function_table;
                if(detail::isHeapAllocated(other.data, other.buffer))
                    data = other.data;
                else
                {
                    buffer = other.buffer;
                    data = &buffer;
                }
                other.data = nullptr;
                return *this;
            }

        private:
            void reset() noexcept
            {
                if(!data)
                    return;

                if(detail::isHeapAllocated(data, buffer))
                    function_table.del(data);
                else
                    function_table.destruct(data);
            }

            void* read() const noexcept
            {
                return data;
            }

            void* write()
            {
                return read();
            }

            void* copy_into(Buffer& other_buffer) const
            {
                if(!data)
                    return nullptr;
                if(detail::isHeapAllocated(data, buffer))
                    return function_table.copy(data);
                return function_table.copy_into(data, other_buffer);
            }

            FunctionTable function_table;
            void* data = nullptr;
            Buffer buffer;
        };


        template <int buffer_size, bool rttiEnabled>
        class NonCopyableSBOStorage : public Accessor< NonCopyableSBOStorage<buffer_size, rttiEnabled>, rttiEnabled >
        {
            using Buffer = std::array<char,buffer_size>;

            struct FunctionTable
            {
                using delete_fn = void(*)(void*);
                using destruct_fn = void(*)(void*);
                delete_fn del = nullptr;
                destruct_fn destruct = nullptr;
            };

            friend class Accessor< NonCopyableSBOStorage, rttiEnabled >;
            friend class Casts< NonCopyableSBOStorage, rttiEnabled >;

            using Base = Accessor< NonCopyableSBOStorage, rttiEnabled >;

        public:
            constexpr NonCopyableSBOStorage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<NonCopyableSBOStorage, std::decay_t<T> >::value>* = nullptr>
            explicit NonCopyableSBOStorage(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  function_table{
                      &detail::deleteData< std::decay_t<T> >,
                      &detail::destructData< std::decay_t<T> > }
            {
                if( sizeof(std::decay_t<T>) <= sizeof(Buffer))
                {
                    new(&buffer) std::decay_t<T>(std::forward<T>(value));
                    data = &buffer;
                }
                else
                    data = new std::decay_t<T>(std::forward<T>(value));
            }

            template <class T,
                      std::enable_if_t<!std::is_base_of<NonCopyableSBOStorage, std::decay_t<T> >::value>* = nullptr>
            NonCopyableSBOStorage& operator=(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
            {
                return *this = NonCopyableSBOStorage(std::forward<T>(value));
            }

            ~NonCopyableSBOStorage()
            {
                reset();
            }

            NonCopyableSBOStorage(NonCopyableSBOStorage&& other) noexcept
                : Base(other),
                  function_table(other.function_table)
            {
                if(!other.data)
                    return;

                if(detail::isHeapAllocated(other.data, other.buffer))
                    data = other.data;
                else
                {
                    buffer = other.buffer;
                    data = &buffer;
                }

                other.data = nullptr;
            }

            NonCopyableSBOStorage& operator=(NonCopyableSBOStorage&& other) noexcept
            {
                reset();
                if(!other.data)
                {
                    data = nullptr;
                    return *this;
                }
                Base::operator=(other);
                function_table = other.function_table;
                if(detail::isHeapAllocated(other.data, other.buffer))
                    data = other.data;
                else
                {
                    buffer = other.buffer;
                    data = &buffer;
                }
                other.data = nullptr;
                return *this;
            }

            NonCopyableSBOStorage(const NonCopyableSBOStorage&) = delete;
            NonCopyableSBOStorage& operator=(const NonCopyableSBOStorage&) = delete;

        private:
            void reset() noexcept
            {
                if(!data)
                    return;

                if(detail::isHeapAllocated(data, buffer))
                    function_table.del(data);
                else
                    function_table.destruct(data);
            }

            void* read() const noexcept
            {
                return data;
            }

            void* write()
            {
                return read();
            }

            FunctionTable function_table;
            void* data = nullptr;
            Buffer buffer;
        };


        template <int buffer_size, bool rttiEnabled>
        class SBOCOWStorage : public Accessor< SBOCOWStorage<buffer_size, rttiEnabled>, rttiEnabled >
        {
            using Buffer = std::array<char,buffer_size>;

            struct FunctionTable
            {
                using destruct_fn = void(*)(void*);
                using copy_fn = std::shared_ptr<void>(*)(const std::shared_ptr<void>&);
                using buffer_copy_fn = std::shared_ptr<void>(*)(const std::shared_ptr<void>&, Buffer&);

                destruct_fn destruct = nullptr;
                copy_fn copy = nullptr;
                buffer_copy_fn copy_into = nullptr;
            };

            friend class Accessor< SBOCOWStorage, rttiEnabled >;
            friend class Casts< SBOCOWStorage, rttiEnabled >;

            using Base = Accessor< SBOCOWStorage, rttiEnabled >;

        public:
            constexpr SBOCOWStorage() noexcept = default;

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                      std::enable_if_t<std::greater<>()(sizeof(std::decay_t<T>), sizeof(Buffer))>* = nullptr>
            explicit SBOCOWStorage(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  function_table{&detail::destructData< std::decay_t<T> >,
                                 &detail::copyData< std::decay_t<T> >,
                                 &detail::copyIntoBuffer<std::decay_t<T>, Buffer>},
                  data(std::make_shared< std::decay_t<T> >(std::forward<T>(value)))
            {
            }

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                      std::enable_if_t<std::less_equal<>()(sizeof(std::decay_t<T>), sizeof(Buffer))>* = nullptr>
            explicit SBOCOWStorage(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
                : Base(Base::template create<std::decay_t<T>>(detail::IsReferenceWrapper< std::decay_t<T> >::value)),
                  function_table{&detail::destructData< std::decay_t<T> >,
                                 &detail::copyData< std::decay_t<T> >,
                                 &detail::copyIntoBuffer<std::decay_t<T>, Buffer>}
            {
                new(&buffer) std::decay_t<T>(std::forward<T>(value));
                data = std::shared_ptr< std::decay_t<T> >(
                           std::shared_ptr< std::decay_t<T> >(),
                           static_cast<std::decay_t<T>*>(static_cast<void*>(&buffer))
                           );
            }

            template <class T,
                      std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr>
            SBOCOWStorage& operator=(T&& value)
            noexcept( sizeof(std::decay_t<T>) <= sizeof(Buffer) &&
                      ( (std::is_rvalue_reference<T>::value && std::is_nothrow_move_constructible<std::decay_t<T>>::value) ||
                        (std::is_lvalue_reference<T>::value && std::is_nothrow_copy_constructible<std::decay_t<T>>::value) ) )
            {
                return *this = SBOCOWStorage(std::forward<T>(value));
            }

            SBOCOWStorage(const SBOCOWStorage& other)
                : Base(other),
                  function_table(other.function_table)
            {
                if(!other.data)
                    return;
                data = other.copy(buffer);
            }

            SBOCOWStorage(SBOCOWStorage&& other) noexcept
                : Base(other),
                  function_table(other.function_table)
            {
                if(!other.data)
                    return;
                data = std::move(other).move_if_heap_allocated(buffer);
                other.data = nullptr;
            }

            ~SBOCOWStorage() noexcept
            {
                reset();
            }

            SBOCOWStorage& operator=(const SBOCOWStorage& other)
            {
                reset();
                if(!other.data)
                {
                    data = nullptr;
                    return *this;
                }
                Base::operator=(other);
                function_table = other.function_table;

                data = other.copy(buffer);
                return *this;
            }

            SBOCOWStorage& operator=(SBOCOWStorage&& other) noexcept
            {
                reset();
                if(!other.data)
                {
                    data = nullptr;
                    return *this;
                }
                Base::operator=(other);
                function_table = other.function_table;
                data = std::move(other).move_if_heap_allocated(buffer);
                other.data = nullptr;
                return *this;
            }

        private:
            void reset() noexcept
            {
                if(!data)
                    return;

                if(!detail::isHeapAllocated(data.get(), buffer))
                    function_table.destruct(data.get());
            }
            void* read() const noexcept
            {
                return data.get();
            }

            void* write()
            {
                if(!data.unique() && detail::isHeapAllocated(data.get(), buffer))
                    data = function_table.copy(data);
                return read();
            }

            std::shared_ptr<void> copy(Buffer& other_buffer) const
            {
                if(detail::isHeapAllocated(data.get(), buffer))
                    return data;
                else
                    return function_table.copy_into(data, other_buffer);
            }

            std::shared_ptr<void> move_if_heap_allocated(Buffer& other_buffer) const &&
            {
                if(detail::isHeapAllocated(data.get(), buffer))
                    return std::move(data);
                else
                    return function_table.copy_into(data, other_buffer);
            }

            FunctionTable function_table;
            std::shared_ptr<void> data = nullptr;
            Buffer buffer;
        };
    }
}
