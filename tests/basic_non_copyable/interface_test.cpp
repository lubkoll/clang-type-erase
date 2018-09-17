#include <gtest/gtest.h>

#include "interface.hh"
#include "../mock_fooable.hh"

namespace
{
    using Basic::Fooable;
    using Mock::MockFooable;

    void death_tests( Fooable& fooable )
    {
#ifndef NDEBUG
        EXPECT_DEATH( fooable.foo(), "" );
        EXPECT_DEATH( fooable.set_value( Mock::other_value ), "" );
#endif
    }

    void test_interface( Fooable& fooable, int initial_value, int new_value )
    {
        EXPECT_EQ( fooable.foo(), initial_value );
        fooable.set_value( new_value );
        EXPECT_EQ( fooable.foo(), new_value );
    }

    void test_ref_interface( Fooable& fooable, const MockFooable& mock_fooable,
                             int new_value )
    {
        test_interface(fooable, mock_fooable.foo(), new_value);
        EXPECT_EQ( mock_fooable.foo(), new_value );
    }

    void test_copies( Fooable& copy, const Fooable& fooable, int new_value )
    {
        auto value = fooable.foo();
        test_interface( copy, value, new_value );
        EXPECT_EQ( fooable.foo(), value );
        ASSERT_NE( value, new_value );
        EXPECT_NE( fooable.foo(), copy.foo() );
    }
}

TEST( TestBasicFooable, Empty )
{
    Fooable fooable;
    death_tests(fooable);

    Fooable copy(fooable);
    death_tests(copy);

    Fooable move( std::move(fooable) );
    death_tests(move);

    Fooable copy_assign;
    copy_assign = move;
    death_tests(copy_assign);

    Fooable move_assign;
    move_assign = std::move(copy_assign);
    death_tests(move_assign);
}

TEST( TestBasicFooable, CopyFromValue )
{
    MockFooable mock_fooable;
    auto value = mock_fooable.foo();
    Fooable fooable( mock_fooable );

    test_interface( fooable, value, Mock::other_value );
}

TEST( TestBasicFooable, CopyConstruction )
{
    Fooable fooable = MockFooable();
    Fooable other( fooable );
    test_copies( other, fooable, Mock::other_value );
}

TEST( TestBasicFooable, CopyFromValueWithReferenceWrapper )
{
    MockFooable mock_fooable;
    Fooable fooable( std::ref(mock_fooable) );

    test_ref_interface( fooable, mock_fooable, Mock::other_value );
}

TEST( TestBasicFooable, MoveFromValue )
{
    MockFooable mock_fooable;
    auto value = mock_fooable.foo();
    Fooable fooable( std::move(mock_fooable) );

    test_interface( fooable, value, Mock::other_value );
}

TEST( TestBasicFooable, MoveConstruction )
{
    Fooable fooable = MockFooable();
    auto value = fooable.foo();
    Fooable other( std::move(fooable) );

    test_interface( other, value, Mock::other_value );
    death_tests(fooable);
}

TEST( TestBasicFooable, MoveFromValueWithReferenceWrapper )
{
    MockFooable mock_fooable;
    Fooable fooable( std::move(std::ref(mock_fooable)) );

    test_ref_interface( fooable, mock_fooable, Mock::other_value );
}

TEST( TestBasicFooable, CopyAssignFromValue )
{
    MockFooable mock_fooable;
    Fooable fooable;

    auto value = mock_fooable.foo();
    fooable = mock_fooable;
    test_interface(fooable, value, Mock::other_value);
}

TEST( TestBasicFooable, CopyAssignment )
{
    Fooable fooable = MockFooable();
    Fooable other;
    other = fooable;
    test_copies( other, fooable, Mock::other_value );
}

TEST( TestBasicFooable, CopyAssignFromValueWithReferenceWrapper )
{
    MockFooable mock_fooable;
    Fooable fooable;

    fooable = std::ref(mock_fooable);
    test_ref_interface( fooable, mock_fooable, Mock::other_value );
}

TEST( TestBasicFooable, MoveAssignFromValue )
{
    MockFooable mock_fooable;
    Fooable fooable;

    auto value = mock_fooable.foo();
    fooable = std::move(mock_fooable);
    test_interface(fooable, value, Mock::other_value);
}

TEST( TestBasicFooable, MoveAssignment )
{
    Fooable fooable = MockFooable();
    auto value = fooable.foo();
    Fooable other;
    other = std::move(fooable);

    test_interface( other, value, Mock::other_value );
    death_tests(fooable);
}

TEST( TestBasicFooable, MoveAssignFromValueWithReferenceWrapper )
{
    MockFooable mock_fooable;
    Fooable fooable;

    fooable = std::move(std::ref(mock_fooable));
    test_ref_interface( fooable, mock_fooable, Mock::other_value );
}

TEST( TestBasicFooable, Cast )
{
    Fooable fooable = MockFooable();

    EXPECT_TRUE( fooable.target<int>() == nullptr );
    ASSERT_FALSE( fooable.target<MockFooable>() == nullptr );
    EXPECT_EQ( fooable.target<MockFooable>()->foo(), Mock::value );
}

TEST( TestBasicFooable, ConstCast )
{
    const Fooable fooable = MockFooable();

    EXPECT_TRUE( fooable.target<int>() == nullptr );
    ASSERT_FALSE( fooable.target<MockFooable>() == nullptr );
    EXPECT_EQ( fooable.target<MockFooable>()->foo(), Mock::value );
}

