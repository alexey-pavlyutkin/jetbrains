#ifndef __JB__PATH_ITERATOR__H__
#define __JB__PATH_ITERATOR__H__


#include "path_utils.h"
#include <string_view>
#include <iterator>
#include <tuple>
#include <assert.h>


namespace jb
{
    namespace details
    {
        template < typename StringT >
        class path_iterator
        {

        public:

            using iterator_category = std::bidirectional_iterator_tag;
            using string_type = StringT;
            using size_type = typename string_type::size_type;
            using value_type = typename string_type::value_type;
            using traits_type = typename string_type::traits_type;
            using difference_type = ptrdiff_t;
            using pointer = value_type const *;
            using reference = value_type const &;
            using self_type = path_iterator< StringT >;
            using view_type = std::basic_string_view< value_type, traits_type >;

        private:

            static constexpr size_type npos = string_type::npos;
            static constexpr value_type separator = '/';

            const string_type * path_ = nullptr;
            size_type position_ = string_type::npos;

        public:

            path_iterator() _NOEXCEPT = default;
            explicit constexpr path_iterator( const string_type * path, size_type position ) _NOEXCEPT : path_( path ), position_( position ) {}
            path_iterator( const self_type & ) _NOEXCEPT = default;
            path_iterator( self_type && ) _NOEXCEPT = default;

            self_type & operator = ( const self_type & other ) noexcept = default;
            self_type & operator = ( self_type && ) noexcept = default;

            self_type & operator++() _NOEXCEPT
            {
                if ( path_ && position_ < path_->size() )
                {
                    position_ = path_->find_first_of( separator, position_ + 1 );
                    position_ = ( position_ != npos ) ? position_ : path_->size();
                }
                else
                {
                    // invalidate the iterator
                    path_ = nullptr;
                    position_ = npos;
                }
                return *this;
            }

            self_type & operator--() _NOEXCEPT
            {
                if ( path_ && 0 < position_ && position_ <= path_->size() )
                {
                    position_ = path_->find_last_of( separator, position_ - 1 );
                    position_ = ( position_ != npos ) ? position_ : 0;
                }
                else
                {
                    // invalidate the iterator
                    path_ = nullptr;
                    position_ = npos;
                }
                return *this;
            }

            self_type operator++( int ) _NOEXCEPT
            {
                auto tmp = *this;
                ++( *this );
                return tmp;
            }

            self_type operator--( int ) _NOEXCEPT
            {
                auto tmp = *this;
                --( *this );
                return tmp;
            }

            reference operator * () _NOEXCEPT
            {
                assert( path_ && position_ < path_->size() );
                return *( path_->begin() + position_ );
            }

            bool is_valid() const _NOEXCEPT
            {
                return path_ && ( position_ == path_->size() || position_ < path_->size() && (*path_)[ position_ ] == separator );
            }

            friend bool operator == ( const self_type & lhs, const self_type & rhs ) _NOEXCEPT
            {
                return lhs.path_ == rhs.path_ && lhs.position_ == rhs.position_;
            }

            friend bool operator != ( const self_type & lhs, const self_type & rhs ) _NOEXCEPT
            {
                return !( lhs == rhs );
            }

            friend view_type operator - ( const self_type & lhs, const self_type & rhs ) _NOEXCEPT
            {
                //
                // the expectations are
                //
                // - both iterators refer valid positions (including END) of the same path
                // - LHS iterator is GE than RHS one (i.e. point the same of greater position)
                //
                assert( lhs.path_ && lhs.path_ == rhs.path_ );
                assert( lhs.position_ <= lhs.path_->size() );
                assert( rhs.position_ <= rhs.path_->size() );
                assert( lhs.position_ >= rhs.position_ );

                if ( lhs != rhs && 
                     lhs.path_ && 
                     lhs.path_ == rhs.path_ && 
                     rhs.position_ < rhs.path_->size() && 
                     rhs.position_ < lhs.position_ &&
                     lhs.position_ <= lhs.path_->size() )
                {
                    auto source = rhs.path_;
                    auto & position = rhs.position_;
                    auto size = lhs.position_ - rhs.position_;

                    assert( position + size <= source->size() );
                    return view_type{ source->data() + position, size }; // does not throw cuz position + size is always in valid range
                }
                else
                {
                    return view_type{};
                }
            }
        };

        template < typename StringT >
        path_iterator< StringT > path_begin( const StringT & path ) _NOEXCEPT
        {
            assert( is_valid_path( path ) );
            return path_iterator< StringT >{ &path, 0 };
        }

        template < typename StringT >
        path_iterator< StringT > path_end( const StringT & path ) _NOEXCEPT
        {
            assert( is_valid_path( path ) );
            return path_iterator< StringT >{ &path, path.size() };
        }
    }
}

#endif
