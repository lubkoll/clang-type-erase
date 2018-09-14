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

            template <class Storage, class Interface, class Derived, bool = std::is_base_of<Interface,Derived>::value>
            struct Cast
            {
                static Derived* target(bool containsReferenceWrapper, Storage* storage) noexcept
                {
                    auto data = storage->getInterfacePtr( );
                    assert(data);
                    if(containsReferenceWrapper)
                        return &dynamic_cast<std::reference_wrapper<Derived>*>(data)->get();
                    return dynamic_cast<Derived*>(data);
                }

                static const Derived* target(bool containsReferenceWrapper, const Storage* storage) noexcept
                {
                    auto data = storage->getInterfacePtr( );
                    assert(data);
                    if(containsReferenceWrapper)
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

            template <class Derived, template <class> class Wrapper, bool rttiEnabled = true>
            class Casts
            {
            public:
                constexpr explicit Casts(bool containsReferenceWrapper = false) noexcept
                    : containsReferenceWrapper(containsReferenceWrapper)
                {}

                template < class T >
                T* target() noexcept
                {
                    auto interface = static_cast<Derived*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
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
                    auto interface = static_cast<const Derived*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
                    {
                        auto wrappedResult = dynamic_cast<const Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = dynamic_cast<const Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }
            protected:
                bool containsReferenceWrapper;
            };


            template <class Derived, template <class> class Wrapper>
            class Casts<Derived, Wrapper, false>
            {
            public:
                constexpr explicit Casts(bool containsReferenceWrapper = false) noexcept
                    : containsReferenceWrapper(containsReferenceWrapper)
                {}

                template < class T >
                T* target() noexcept
                {
                    auto interface = static_cast<const Derived*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
                    {
                        auto wrappedResult = static_cast<Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = static_cast<Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }

                template < class T >
                const T* target() const noexcept
                {
                    auto interface = static_cast<const Derived*>(this)->getInterfacePtr();
                    if(containsReferenceWrapper)
                    {
                        auto wrappedResult = static_cast<const Wrapper<std::reference_wrapper<T>>*>(interface);
                        return wrappedResult ? &wrappedResult->impl : nullptr;
                    }
                    auto wrappedResult = static_cast<const Wrapper<T>*>(interface);
                    return wrappedResult ? &wrappedResult->impl : nullptr;
                }
            protected:
                bool containsReferenceWrapper;
            };


            template <class Storage, class Interface, template <class> class Wrapper>
            class Accessor : public Casts<Storage, Wrapper>
            {
                using Casts<Storage, Wrapper>::containsReferenceWrapper;
            public:
                constexpr explicit Accessor(bool containsReferenceWrapper = false) noexcept
                    : Casts<Storage, Wrapper>(containsReferenceWrapper)
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

                explicit operator bool() const noexcept
                {
                    return static_cast<const Storage*>(this)->getInterfacePtr( ) != nullptr;
                }
            };

            template <class Interface, template <class> class Wrapper>
            struct Storage : Accessor<Storage<Interface,Wrapper>, Interface, Wrapper>
            {
                Storage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<Storage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                explicit Storage(T&& t)
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
                friend class Casts<Storage, Wrapper>;

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
                COWStorage() = default;

                template <class T,
                          std::enable_if_t<!std::is_base_of<COWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr>
                explicit COWStorage(T&& t)
                    : interface_(std::make_shared<Wrapper<T>>(std::forward<T>(t)))
                {}

            private:
                friend class Accessor<COWStorage, Interface, Wrapper>;
                friend class Casts<COWStorage, Wrapper>;

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
                    : interface_(std::make_shared<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::less_equal<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOStorage(T&& t)
                {
                    new(&buffer_) Wrapper<std::decay_t<T>>(std::forward<T>(t));
                    interface_ = makeAlias();
                }

                SBOStorage(const SBOStorage& other)
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
                friend class Casts<SBOStorage, Wrapper>;

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
                    : interface_(std::make_shared<Wrapper<std::decay_t<T>>>(std::forward<T>(t)))
                {
                }

                template <class T,
                          std::enable_if_t<!std::is_base_of<SBOCOWStorage, std::decay_t<T> >::value>* = nullptr,
                          std::enable_if_t<std::is_base_of<Interface, Wrapper<T>>::value>* = nullptr,
                          std::enable_if_t<std::less_equal<>()(sizeof(Wrapper<std::decay_t<T>>), Size)>* = nullptr>
                explicit SBOCOWStorage(T&& t)
                {
                    new(&buffer_) Wrapper<std::decay_t<T>>(std::forward<T>(t));
                    interface_ = makeAlias();
                }

                SBOCOWStorage(const SBOCOWStorage& other)
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
                friend class Casts<SBOCOWStorage, Wrapper>;

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
