// This file was automatically generated using clang-type-erase.
// Please do not modify.

#pragma once

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>

#ifdef CLANG_TYPE_ERASE_CAST
#undef CLANG_TYPE_ERASE_CAST
#endif

#ifdef CLANG_TYPE_ERASE_NO_RTTI
#define CLANG_TYPE_ERASE_CAST static_cast
#else
#define CLANG_TYPE_ERASE_CAST dynamic_cast
#endif

namespace clang
{
    namespace type_erasure
    {
        namespace polymorphic
        {
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

            template <class Storage, class Interface, template <class> class Wrapper>
            class Accessor
            {
            public:
                constexpr explicit Accessor(bool containsReferenceWrapper = false) noexcept
                    : containsReferenceWrapper(containsReferenceWrapper)
                {}

                Interface* operator->()
                {
                    return static_cast<Storage*>(this)->getInterfacePtr( );
                }

                const Interface* operator->() const
                {
                    return static_cast<const Storage*>(this)->getInterfacePtr( );
                }

                template <class T>
                T& get() noexcept
                {
                    auto data = static_cast<Storage*>(this)->getInterfacePtr( );
                    assert(data);
                    if(containsReferenceWrapper)
                        return static_cast<std::reference_wrapper<T>*>(data)->get();
                    return *static_cast<T*>(data);
                }

                template <class T>
                const T& get() const noexcept
                {
                    const auto data = static_cast<const Storage*>(this)->getInterfacePtr( );
                    assert(data);
                    if(containsReferenceWrapper)
                        return static_cast<const std::reference_wrapper<T>*>(data)->get();
                    return *static_cast<const T*>(data);
                }

                template < class T >
                T* target() noexcept
                {
                    auto interface = static_cast<Storage*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
                    {
                        auto wrappedResult = CLANG_TYPE_ERASE_CAST<Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = CLANG_TYPE_ERASE_CAST<Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }

                template < class T >
                const T* target() const noexcept
                {
                    auto interface = static_cast<const Storage*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
                    {
                        auto wrappedResult = CLANG_TYPE_ERASE_CAST<const Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = CLANG_TYPE_ERASE_CAST<const Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }

                explicit operator bool() const noexcept
                {
                    return static_cast<const Storage*>(this)->getInterfacePtr( ) != nullptr;
                }

            private:
                bool containsReferenceWrapper;
            };

            template <class Interface, template <class> class Wrapper>
            struct Storage : Accessor<Storage<Interface,Wrapper>, Interface, Wrapper>
            {
                using Base = Accessor<Storage, Interface, Wrapper>;

                Storage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<Storage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                explicit Storage(T&& t)
                    : Base()
                    ,interface_(std::make_unique<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {}

                Storage(Storage&&) = default;
                Storage& operator=(Storage&&) = default;

                Storage(const Storage& other)
                    : Base()
                    , interface_(other.interface_ ? other.interface_->clone() : nullptr)
                {}

                Storage& operator=(const Storage& other)
                {
                    interface_ = other.interface_ ? other.interface_->clone() : nullptr;
                    return *this;
                }

            private:
                friend class Accessor<Storage, Interface, Wrapper>;

                Interface* getInterfacePtr()
                {
                    return interface_.get();
                }

                const Interface* getInterfacePtr() const
                {
                    return interface_.get();
                }

                std::unique_ptr<Interface> interface_;
            };

            template <class Interface, template <class> class Wrapper>
            struct COWStorage : Accessor<COWStorage<Interface,Wrapper>, Interface, Wrapper>
            {
                using Base = Accessor<COWStorage, Interface, Wrapper>;

                COWStorage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<COWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                explicit COWStorage(T&& t)
                    : Base()
                    , interface_(std::make_shared<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {}

            private:
                friend class Accessor<COWStorage, Interface, Wrapper>;

                Interface* getInterfacePtr()
                {
                    if(!interface_.unique())
                        interface_ = interface_->clone();
                    return interface_.get();
                }

                const Interface* getInterfacePtr() const
                {
                    return interface_.get();
                }

                std::shared_ptr<Interface> interface_;
            };


            template <class Interface, template <class> class Wrapper, int Size>
            struct SBOStorage : Accessor<SBOStorage<Interface,Wrapper,Size>, Interface, Wrapper>
            {
                using Base = Accessor<SBOStorage, Interface, Wrapper>;

                SBOStorage() = default;

                ~SBOStorage()
                {
                    reset();
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::greater<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOStorage(T&& t)
                    : Base()
                    , interface_(std::make_shared<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::less_equal<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOStorage(T&& t)
                    : Base()
                {
                    new(&buffer_) Wrapper<std::decay_t<T>>(std::forward<T>(t));
                    interface_ = makeAlias();
                }

                SBOStorage(const SBOStorage& other)
                    : Base()
                {
                    if(!other.interface_)
                        return;
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_->clone();
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                }

                SBOStorage& operator=(const SBOStorage& other)
                {
                    reset();
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_ ? other.interface_->clone() : nullptr;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    return *this;
                }

                SBOStorage(SBOStorage&& other)
                {
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    other.interface_ = nullptr;
                }

                SBOStorage& operator=(SBOStorage&& other)
                {
                    reset();
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    other.interface_ = nullptr;
                    return *this;
                }

            private:
                friend class Accessor<SBOStorage, Interface, Wrapper>;

                Interface* getInterfacePtr()
                {
                    return interface_.get();
                }

                const Interface* getInterfacePtr() const
                {
                    return interface_.get();
                }

                void reset()
                {
                    if(!isHeapAllocated(interface_.get(), buffer_))
                        interface_->~Interface();
                }

                std::shared_ptr<Interface> makeAlias()
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
                using Base = Accessor<SBOCOWStorage, Interface, Wrapper>;

                SBOCOWStorage() = default;

                ~SBOCOWStorage()
                {
                    reset();
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::greater<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOCOWStorage(T&& t)
                    : Base()
                    , interface_(std::make_shared<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::less_equal<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOCOWStorage(T&& t)
                    : Base()
                {
                    new(&buffer_) Wrapper<std::decay_t<T>>(std::forward<T>(t));
                    interface_ = makeAlias();
                }

                SBOCOWStorage(const SBOCOWStorage& other)
                    : Base()
                {
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                }

                SBOCOWStorage& operator=(const SBOCOWStorage& other)
                {
                    reset();
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = other.interface_;
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    return *this;
                }

                SBOCOWStorage(SBOCOWStorage&& other)
                    : Base()
                {
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    other.interface_ = nullptr;
                }

                SBOCOWStorage& operator=(SBOCOWStorage&& other)
                {
                    reset();
                    if(isHeapAllocated(other.interface_.get(), other.buffer_)) {
                        interface_ = std::move(other.interface_);
                    } else {
                        buffer_ = other.buffer_;
                        interface_ = makeAlias();
                    }
                    other.interface_ = nullptr;
                    return *this;
                }

            private:
                friend class Accessor<SBOCOWStorage, Interface, Wrapper>;

                Interface* getInterfacePtr()
                {
                    if(isHeapAllocated(interface_.get(), buffer_) && !interface_.unique())
                        interface_ = interface_->clone();
                    return interface_.get();
                }

                const Interface* getInterfacePtr() const
                {
                    return interface_.get();
                }

                void reset()
                {
                    if(!isHeapAllocated(interface_.get(), buffer_))
                        interface_->~Interface();
                }

                std::shared_ptr<Interface> makeAlias()
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

#undef CLANG_TYPE_ERASE_CAST
