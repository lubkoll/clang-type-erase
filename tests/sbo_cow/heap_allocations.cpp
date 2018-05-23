#include <gtest/gtest.h>

#include "interface.hh"
#include "../mock_fooable.hh"
#include "../util.hh"

using SBO_COW::Fooable;
using Mock::MockFooable;
using Mock::MockLargeFooable;

TEST( TestSBOCOWFooable_HeapAllocations, Empty )
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
                      move_assign = std::move(fooable),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyFromValue_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( mock_fooable ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyConstruction_SmallObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other( fooable ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyFromValueWithReferenceWrapper_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::ref(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveFromValue_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveConstruction_SmallObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other( std::move(fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveFromValueWithReferenceWrapper_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(std::ref(mock_fooable)) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignFromValue_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = mock_fooable,
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignment_SmallObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = fooable,
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignFromValuenWithReferenceWrapper_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::ref(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignFromValue_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignment_SmallObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = std::move(fooable),
                      expected_heap_allocations );
}


TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignFromValueWithReferenceWrapper_SmallObject )
{
    auto expected_heap_allocations = 0u;

    MockFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(std::ref(mock_fooable)),
                      expected_heap_allocations );
}


TEST( TestSBOCOWFooable_HeapAllocations, CopyFromValue_LargeObject )
{
    auto expected_heap_allocations = 1u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( mock_fooable ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyConstruction_LargeObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockLargeFooable();
    CHECK_HEAP_ALLOC( Fooable other( fooable ),
                      expected_heap_allocations );

    expected_heap_allocations = 1u;
    CHECK_HEAP_ALLOC( other.set_value(Mock::other_value),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyFromValueWithReferenceWrapper_LargeObject )
{
    auto expected_heap_allocations = 0u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::ref(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveFromValue_LargeObject )
{
    auto expected_heap_allocations = 1u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(mock_fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveConstruction_LargeObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockLargeFooable();
    CHECK_HEAP_ALLOC( Fooable other( std::move(fooable) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveFromValueWithReferenceWrapper_LargeObject )
{
    auto expected_heap_allocations = 0u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable( std::move(std::ref(mock_fooable)) ),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignFromValue_LargeObject )
{
    auto expected_heap_allocations = 1u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = mock_fooable,
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignment_LargeObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockLargeFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = fooable,
                      expected_heap_allocations );

    expected_heap_allocations = 1u;
    CHECK_HEAP_ALLOC( other.set_value(Mock::other_value),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, CopyAssignFromValuenWithReferenceWrapper_LargeObject )
{
    auto expected_heap_allocations = 0u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::ref(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignFromValue_LargeObject )
{
    auto expected_heap_allocations = 1u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(mock_fooable),
                      expected_heap_allocations );
}

TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignment_LargeObject )
{
    auto expected_heap_allocations = 0u;

    Fooable fooable = MockLargeFooable();
    CHECK_HEAP_ALLOC( Fooable other;
                      other = std::move(fooable),
                      expected_heap_allocations );
}


TEST( TestSBOCOWFooable_HeapAllocations, MoveAssignFromValueWithReferenceWrapper_LargeObject )
{
    auto expected_heap_allocations = 0u;

    MockLargeFooable mock_fooable;
    CHECK_HEAP_ALLOC( Fooable fooable;
                      fooable = std::move(std::ref(mock_fooable)),
                      expected_heap_allocations );
}
