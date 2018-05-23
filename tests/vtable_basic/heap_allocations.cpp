#include <gtest/gtest.h>

#include "interface.hh"
#include "../mock_fooable.hh"
#include "../util.hh"

namespace
{
    using VTableBasic::Fooable;
    using Mock::MockFooable;
}

TEST( TestVTableBasicFooable_HeapAllocations, Empty )
{
    auto expected_heap_allocations = 0u;

    CHECK_HEAP_ALLOC( Fooable fooable,
                      expected_heap_allocations );

    CHECK_HEAP_ALLOC( Fooable copy(fooable),
                      expected_heap_allocations );

    CHECK_HEAP_ALLOC( Fooable move( std::move(fooable) ),
                      expected_heap_allocations );

    CHECK_HEAP_ALLOC( Fooable copy_assign;
                      copy_assign = move,
                      expected_heap_allocations );

    CHECK_HEAP_ALLOC( Fooable move_assign;
                      move_assign = std::move(copy_assign),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyFromValue )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( mock_fooable ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyConstruction )
{
    auto expected_heap_allocations = 1u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other( fooable ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyFromValueWithReferenceWrapper )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::ref(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, MoveFromValue )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, MoveConstruction )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other( std::move(fooable) ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, MoveFromValueWithReferenceWrapper )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(std::ref(mock_fooable)) ),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyAssignFromValue )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = mock_fooable,
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyAssignment )
{
    auto expected_heap_allocations = 1u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = fooable,
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, CopyAssignFromValuenWithReferenceWrapper )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::ref(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, MoveAssignFromValue )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestVTableBasicFooable_HeapAllocations, MoveAssignment )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = std::move(fooable),
                      expected_heap_allocations );
}


TEST( TestVTableBasicFooable_HeapAllocations, MoveAssignFromValueWithReferenceWrapper )
{
    auto expected_heap_allocations = 1u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(std::ref(mock_fooable)),
                      expected_heap_allocations );
}
