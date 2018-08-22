#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <type_traits>
#include <iostream>

namespace clang
{
    namespace type_erasure
    {
        namespace polymorphic
        {
            inline const char* char_ptr( const void* ptr ) noexcept
            {
                assert(ptr);
                return static_cast<const char*>( ptr );
            }

            template < class Buffer >
            bool is_heap_allocated (void* data, const Buffer& buffer) noexcept
            {
                // treat nullptr as heap-allocated
                if(!data)
                    return true;
                return data < static_cast<const void*>(&buffer) ||
                        static_cast<const void*>( char_ptr(&buffer) + sizeof(buffer) ) <= data;
            }

            template <class Storage, class Interface, class Derived, bool = std::is_base_of<Interface,Derived>::value>
            struct Cast
            {
                static Derived* target(bool contains_reference_wrapper, Storage* storage) noexcept
                {
                    auto data = storage->get_interface_ptr( );
                    assert(data);
                    if(contains_reference_wrapper)
                        return &dynamic_cast<std::reference_wrapper<Derived>*>(data)->get();
                    return dynamic_cast<Derived*>(data);
                }

                static const Derived* target(bool contains_reference_wrapper, const Storage* storage) noexcept
                {
                    auto data = storage->get_interface_ptr( );
                    assert(data);
                    if(contains_reference_wrapper)
                        return &dynamic_cast<const std::reference_wrapper<Derived>*>(data)->get();
                    return dynamic_cast<const Derived*>(data);
                }
            };

            template <class Storage, class Interface, class Derived>
            struct Cast<Storage, Interface, Derived, false>
            {
                static Derived* target(bool, Storage*) noexcept
                {
                    return nullptr;
                }

                static const Derived* target(bool, const Storage*) noexcept
                {
                    return nullptr;
                }
            };

            template <class Storage, class Interface, template <class> class Wrapper>
            class Accessor
            {
            public:
                constexpr explicit Accessor(bool contains_ref_wrapper = false) noexcept
                    : contains_reference_wrapper(contains_ref_wrapper)
                {}

                Interface* operator->()
                {
                    return static_cast<Storage*>(this)->get_interface_ptr( );
                }

                const Interface* operator->() const
                {
                    return static_cast<const Storage*>(this)->get_interface_ptr( );
                }

                template <class T>
                T& get() noexcept
                {
                    auto data = static_cast<Storage*>(this)->get_interface_ptr( );
                    assert(data);
                    if(contains_reference_wrapper)
                        return static_cast<std::reference_wrapper<T>*>(data)->get();
                    return *static_cast<T*>(data);
                }

                template <class T>
                const T& get() const noexcept
                {
                    const auto data = static_cast<const Storage*>(this)->get_interface_ptr( );
                    assert(data);
                    if(contains_reference_wrapper)
                        return static_cast<const std::reference_wrapper<T>*>(data)->get();
                    return *static_cast<const T*>(data);
                }

                template < class T >
                T* target() noexcept
                {
                    auto interface = static_cast<const Storage*>(this)->interface_.get();
                    if(contains_reference_wrapper)
                    {
                        auto wrappedResult = dynamic_cast<Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = dynamic_cast<Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }

                template < class T >
                const T* target() const noexcept
                {
                    auto interface = static_cast<const Storage*>(this)->interface_.get();
                    if(contains_reference_wrapper)
                    {
                        auto wrappedResult = dynamic_cast<const Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = dynamic_cast<const Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }

                explicit operator bool() const noexcept
                {
                    return static_cast<const Storage*>(this)->get_interface_ptr( ) != nullptr;
                }

            private:
                bool contains_reference_wrapper = false;
            };

            template <class Interface, template <class> class Wrapper>
            struct Storage : Accessor<Storage<Interface,Wrapper>, Interface, Wrapper>
            {
                Storage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<Storage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                Storage(T&& t)
                    : interface_(std::make_unique<Wrapper<T>>(std::forward<T>(t)))
                {}

                Storage(Storage&&) = default;
                Storage& operator=(Storage&&) = default;

                Storage(const Storage& other)
                    : interface_(other.interface_ ? other.interface_->clone() : nullptr)
                {}

                Storage& operator=(const Storage& other)
                {
                    interface_ = other.interface_ ? other.interface_->clone() : nullptr;
                    return *this;
                }

            private:
                friend class Accessor<Storage, Interface, Wrapper>;

                Interface* get_interface_ptr()
                {
                    return interface_.get();
                }

                const Interface* get_interface_ptr() const
                {
                    return interface_.get();
                }

                std::unique_ptr<Interface> interface_;
            };

            template <class Interface, template <class> class Wrapper>
            struct COWStorage : Accessor<COWStorage<Interface,Wrapper>, Interface, Wrapper>
            {
                COWStorage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<COWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                COWStorage(T&& t)
                    : interface_(std::make_shared<Wrapper<T>>(std::forward<T>(t)))
                {}

            private:
                Interface* get_interface_ptr()
                {
                    if(!interface_.unique())
                        interface_ = interface_->clone();
                    return interface_.get();
                }

                const Interface* get_interface_ptr() const
                {
                    return interface_.get();
                }

                friend class Accessor<COWStorage, Interface, Wrapper>;

                std::shared_ptr<Interface> interface_;
            };


            template <class Interface, template <class> class Wrapper, int Size>
            struct SBOStorage : Accessor<SBOStorage<Interface,Wrapper,Size>, Interface, Wrapper>
            {
                SBOStorage() = default;

                ~SBOStorage()
                {
                    reset();
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                SBOStorage(T&& t)
                {
                    using DecT = std::decay_t<T>;
                    if(sizeof(Wrapper<DecT>) > Size) {
                        interface_ = std::make_shared<Wrapper<DecT>>(std::forward<T>(t));
                    } else {
                        new(&buffer_) Wrapper<DecT>(std::forward<T>(t));
                        interface_ = make_alias();
                    }
                }

                SBOStorage(const SBOStorage& other)
                {
                    if(!other.interface_)
                        return;
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_->clone();
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                }

                SBOStorage& operator=(const SBOStorage& other)
                {
                    reset();
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_ ? other.interface_->clone() : nullptr;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    return *this;
                }

                SBOStorage(SBOStorage&& other)
                {
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    other.interface_ = nullptr;
                }

                SBOStorage& operator=(SBOStorage&& other)
                {
                    reset();
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    other.interface_ = nullptr;
                    return *this;
                }

            private:
                friend class Accessor<SBOStorage, Interface, Wrapper>;

                Interface* get_interface_ptr()
                {
                    return interface_.get();
                }

                const Interface* get_interface_ptr() const
                {
                    return interface_.get();
                }

                void reset()
                {
                    if(!is_heap_allocated(interface_.get(), buffer_))
                        interface_->~Interface();
                }

                std::shared_ptr<Interface> make_alias()
                {
                    const auto tmp = static_cast<Interface*>(static_cast<void*>(&buffer_[0]));
                    return std::shared_ptr<Interface>(std::shared_ptr<Interface>(), tmp);
                }

                std::array<char,Size> buffer_;
                std::shared_ptr<Interface> interface_ = nullptr;
            };


            template <class Interface, template <class> class Wrapper, int Size>
            struct SBOCOWStorage : Accessor<SBOCOWStorage<Interface,Wrapper,Size>, Interface, Wrapper>
            {
                SBOCOWStorage() = default;

                ~SBOCOWStorage()
                {
                    reset();
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                SBOCOWStorage(T&& t)
                {
                    using DecT = std::decay_t<T>;
                    if(sizeof(Wrapper<DecT>) > Size) {
                        interface_ = std::make_shared<Wrapper<DecT>>(std::forward<T>(t));
                    } else {
                        new(&buffer_) Wrapper<DecT>(std::forward<T>(t));
                        interface_ = make_alias();
                    }
                }

                SBOCOWStorage(const SBOCOWStorage& other)
                {
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                }

                SBOCOWStorage& operator=(const SBOCOWStorage& other)
                {
                    reset();
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    return *this;
                }

                SBOCOWStorage(SBOCOWStorage&& other)
                {
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    other.interface_ = nullptr;
                }

                SBOCOWStorage& operator=(SBOCOWStorage&& other)
                {
                    reset();
                    if(is_heap_allocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = make_alias();
                    }
                    other.interface_ = nullptr;
                    return *this;
                }

            private:
                friend class Accessor<SBOCOWStorage, Interface, Wrapper>;

                Interface* get_interface_ptr()
                {
                    if(is_heap_allocated(interface_.get(), buffer_) && !interface_.unique())
                        interface_ = interface_->clone();
                    return interface_.get();
                }

                const Interface* get_interface_ptr() const
                {
                    return interface_.get();
                }

                void reset()
                {
                    if(!is_heap_allocated(interface_.get(), buffer_))
                        interface_->~Interface();
                }

                std::shared_ptr<Interface> make_alias()
                {
                    const auto tmp = static_cast<Interface*>(static_cast<void*>(&buffer_[0]));
                    return std::shared_ptr<Interface>(std::shared_ptr<Interface>(), tmp);
                }

                std::array<char,Size> buffer_;
                std::shared_ptr<Interface> interface_ = nullptr;
            };
        }
    }
}
