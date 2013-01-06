/*=============================================================================
    Copyright (c) 2001-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_SEQUENCE_JAN_06_2012_1015AM)
#define SPIRIT_SEQUENCE_JAN_06_2012_1015AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/end.hpp>
#include <boost/fusion/include/advance.hpp>
#include <boost/fusion/include/empty.hpp>
#include <boost/fusion/include/front.hpp>
#include <boost/fusion/include/iterator_range.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace spirit { namespace x3
{
    namespace detail
    {
        template <typename L, typename R, typename Attribute>
        struct partition_attribute;

        template <typename Parser, typename Attribute>
        struct pass_sequence_attribute;
    }

    template <typename Left, typename Right>
    struct sequence : parser<sequence<Left, Right>>
    {
        typedef Left left_type;
        typedef Right right_type;
        static bool const has_attribute =
            left_type::has_attribute || right_type::has_attribute;

        sequence(Left left, Right right)
            : left(left), right(right) {}

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(
            Iterator& first, Iterator const& last
          , Context& context, Attribute& attr) const
        {
            typedef detail::partition_attribute<Left, Right, Attribute> partition;
            auto left_seq = partition::left(attr);
            auto right_seq = partition::right(attr);

            typedef detail::pass_sequence_attribute<
                Left, typename partition::l_range>
            l_pass;

            typedef detail::pass_sequence_attribute<
                Right, typename partition::r_range>
            r_pass;

            typename l_pass::type l_attr = l_pass::call(left_seq);
            typename r_pass::type r_attr = r_pass::call(right_seq);

            return left.parse(first, last, context, l_attr)
               && right.parse(first, last, context, r_attr);
        }

        template <typename Iterator, typename Context>
        bool parse(
            Iterator& first, Iterator const& last
          , Context& context, unused_type) const
        {
            return left.parse(first, last, context, unused)
               && right.parse(first, last, context, unused);
        }

        left_type left;
        right_type right;
    };

    template <typename Left, typename Right>
    inline sequence<
        typename extension::as_parser<Left>::value_type
      , typename extension::as_parser<Right>::value_type>
    operator>>(Left const& left, Right const& right)
    {
        typedef sequence<
            typename extension::as_parser<Left>::value_type
          , typename extension::as_parser<Right>::value_type>
        result_type;

        return result_type(as_parser(left), as_parser(right));
    }

    namespace detail
    {
        template <typename Parser>
        struct num_elements
        {
            static int const value = Parser::has_attribute;
        };

        template <typename L, typename R>
        struct num_elements<sequence<L, R>>
        {
            static int const value =
                num_elements<L>::value + num_elements<R>::value;
        };

        template <typename L, typename R, typename Attribute>
        struct partition_attribute
        {
            static int const l_size = num_elements<L>::value;
            static int const r_size = num_elements<R>::value;

            // If you got an error here, then you are trying to pass
            // a fusion sequence with the wrong number of elements
            // as expected by the (sequence) parser.
            static_assert(
                fusion::result_of::size<Attribute>::value == (l_size + r_size)
              , "Attribute does not have the expected size."
            );

            typedef typename fusion::result_of::begin<Attribute>::type l_begin;
            typedef typename fusion::result_of::advance_c<l_begin, l_size>::type l_end;
            typedef typename fusion::result_of::end<Attribute>::type r_end;

            typedef fusion::iterator_range<l_begin, l_end> l_range;
            typedef fusion::iterator_range<l_end, r_end> r_range;

            static l_range left(Attribute& s)
            {
                auto i = fusion::begin(s);
                return l_range(i, fusion::advance_c<l_size>(i));
            }

            static r_range right(Attribute& s)
            {
                return r_range(
                    fusion::advance_c<l_size>(fusion::begin(s))
                  , fusion::end(s));
            }
        };

        template <typename Parser, typename Attribute>
        struct pass_sequence_attribute_unused
        {
            typedef unused_type type;

            template <typename Attribute_>
            static unused_type
            call(Attribute_& attr)
            {
                return unused_type();
            }
        };

        template <typename Parser, typename Attribute>
        struct pass_sequence_attribute_used
        {
            typedef typename fusion::result_of::front<Attribute>::type type;

            template <typename Attribute_>
            static typename add_reference<type>::type
            call(Attribute_& attr)
            {
                return fusion::front(attr);
            }
        };

        template <typename Parser, typename Attribute>
        struct pass_sequence_attribute :
            mpl::if_<fusion::result_of::empty<Attribute>
              , pass_sequence_attribute_unused<Parser, Attribute>
              , pass_sequence_attribute_used<Parser, Attribute>>::type
        {
        };

        template <typename L, typename R, typename Attribute>
        struct pass_sequence_attribute<sequence<L, R>, Attribute>
        {
            typedef Attribute type;

            template <typename Attribute_>
            static Attribute_&
            call(Attribute_& attr)
            {
                return attr;
            }
        };

    }
}}}

#endif
