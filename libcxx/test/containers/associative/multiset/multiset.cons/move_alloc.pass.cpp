//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <set>

// class multiset

// multiset(multiset&& s, const allocator_type& a);

#include <set>
#include <cassert>

#include "../../../MoveOnly.h"
#include "../../../test_compare.h"
#include "../../../test_allocator.h"

int main()
{
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    {
        typedef MoveOnly V;
        typedef test_compare<std::less<MoveOnly> > C;
        typedef test_allocator<V> A;
        typedef std::multiset<MoveOnly, C, A> M;
        typedef std::move_iterator<V*> I;
        V a1[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m1(I(a1), I(a1+sizeof(a1)/sizeof(a1[0])), C(5), A(7));
        V a2[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m2(I(a2), I(a2+sizeof(a2)/sizeof(a2[0])), C(5), A(7));
        M m3(std::move(m1), A(7));
        assert(m3 == m2);
        assert(m3.get_allocator() == A(7));
        assert(m3.key_comp() == C(5));
        assert(m1.empty());
    }
    {
        typedef MoveOnly V;
        typedef test_compare<std::less<MoveOnly> > C;
        typedef test_allocator<V> A;
        typedef std::multiset<MoveOnly, C, A> M;
        typedef std::move_iterator<V*> I;
        V a1[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m1(I(a1), I(a1+sizeof(a1)/sizeof(a1[0])), C(5), A(7));
        V a2[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m2(I(a2), I(a2+sizeof(a2)/sizeof(a2[0])), C(5), A(7));
        M m3(std::move(m1), A(5));
        assert(m3 == m2);
        assert(m3.get_allocator() == A(5));
        assert(m3.key_comp() == C(5));
        assert(m1.empty());
    }
    {
        typedef MoveOnly V;
        typedef test_compare<std::less<MoveOnly> > C;
        typedef other_allocator<V> A;
        typedef std::multiset<MoveOnly, C, A> M;
        typedef std::move_iterator<V*> I;
        V a1[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m1(I(a1), I(a1+sizeof(a1)/sizeof(a1[0])), C(5), A(7));
        V a2[] =
        {
            V(1),
            V(1),
            V(1),
            V(2),
            V(2),
            V(2),
            V(3),
            V(3),
            V(3)
        };
        M m2(I(a2), I(a2+sizeof(a2)/sizeof(a2[0])), C(5), A(7));
        M m3(std::move(m1), A(5));
        assert(m3 == m2);
        assert(m3.get_allocator() == A(5));
        assert(m3.key_comp() == C(5));
        assert(m1.empty());
    }
#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
}
