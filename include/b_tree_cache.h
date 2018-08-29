#ifndef __JB__PHYSICAL_STORAGE__H__
#define __JB__PHYSICAL_STORAGE__H__


#include <unordered_map>
#include <list>
#include <exception>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace jb
{
    /** Implements volume <--> file data exchange

    The main purpose for the class is to assure correct sharing of B tree nodes (that is actually
    how the data represented on physical level). The class guaranties that each B tree node from
    a physical file has the only reflection on logical level.

    @tparam Policies - global setting
    @tparam Pad - test pad
    */
    template < typename Policies >
    class Storage< Policies >::PhysicalVolumeImpl::BTreeCache
    {
        using BTree = typename PhysicalVolumeImpl::BTree;
        using BTreeP = typename BTree::BTreeP;
        using NodeUid = typename BTree::NodeUid;
        using StorageFile = typename PhysicalVolumeImpl::StorageFile;
        using shared_lock = boost::upgrade_lock< boost::upgrade_mutex >;
        using exclusive_lock = boost::upgrade_to_unique_lock< boost::upgrade_mutex >;

        RetCode status_ = RetCode::Ok;
        StorageFile * file_;

        using MruOrder = std::list< NodeUid >;
        using MruItems = std::unordered_map< NodeUid, std::pair< BTreeP, typename MruOrder::iterator > >;

        boost::upgrade_mutex mru_mutex_;
        MruOrder mru_order_;
        MruItems mru_items_;
        static constexpr auto CacheSize = Policies::PhysicalVolumePolicy::BTreeCacheSize;


    public:

        /** The class is not default constructible
        */
        BTreeCache( ) = delete;


        /** The class is not copyable/movable
        */
        BTreeCache( BTreeCache&& ) = delete;


        /** Constructs physical storage

        @param [in] file - pointer to storage file
        @throw nothing
        @note check object validity by status()
        */
        explicit BTreeCache( StorageFile * file ) try
            : file_( file )
            , mru_order_( CacheSize, InvalidNodeUid )
            , mru_items_( CacheSize )
        {
        }
        catch ( const std::bad_alloc & )
        {
            status_ = RetCode::InsufficientMemory;
        }
        catch ( ... )
        {
            status_ = RetCode::UnknownError;
        }


        /** Provides object status

        @retval RetCode - creation status
        @throw nothing
        */
        auto status() const noexcept { return status_; }


        /** Provides requested B-tree node from MRU cache

        @param [in] uid - node UID
        @retval RetCode - operation status
        @retval BTreeP - if operation succeeds holds shared pointer to requested item
        @throw nothing
        */
        [[nodiscard]]
        std::tuple< RetCode, BTreeP > get_node( NodeUid uid ) noexcept
        {
            using namespace std;

            try
            {
                // shared lock over the cache
                shared_lock s{ mru_mutex_ };

                if ( auto item_it = mru_items_.find( uid ); item_it != end( mru_items_ ) )
                {
                    // get exclusive lock over the cache
                    exclusive_lock e{ s };

                    // mark the item as MRU
                    mru_order_.splice( end( mru_order_ ), mru_order_, item_it->second.second );

                    return { RetCode::Ok, item_it->second.first };
                }
                else
                {
                    assert( file_ );

                    auto p = make_shared< BTree >( uid, file_, this );
                    auto isbuf = file_->get_chain_reader( uid );
                    istream is( &isbuf );
                    boost::archive::binary_iarchive ar( is );
                    ar & *p;

                    // through the order list
                    for ( auto order_it = begin( mru_order_ ); order_it != end( mru_order_ ); ++order_it )
                    {
                        // is free?
                        if ( InvalidNodeUid == *order_it )
                        {
                            // get exclusive lock over the cache
                            exclusive_lock e{ s };

                            // insert new item and mark is at MRU
                            mru_items_.emplace( uid, move( pair{ p, order_it } ) );
                            mru_order_.splice( end( mru_order_ ), mru_order_, order_it );
                            *order_it = uid;

                            return { RetCode::Ok, p };
                        }

                        // if item is not used anymore
                        if ( auto item_it = mru_items_.find( *order_it ); item_it->second.first.use_count( ) == 1 )
                        {
                            // get exclusive lock over the cache
                            exclusive_lock e{ s };

                            // drop useless item from cache
                            mru_items_.erase( item_it );

                            // insert new item and mark as MRU
                            mru_items_.emplace( uid, move( pair{ p, order_it } ) );
                            mru_order_.splice( end( mru_order_ ), mru_order_, order_it );
                            *order_it = uid;

                            return { RetCode::Ok, p };
                        }
                    }

                    return { RetCode::TooManyConcurrentOps, BTreeP{} };
                }
            }
            catch ( const std::exception & e )
            {
                cout << e.what() << endl;
            }
            catch ( ... )
            {
            }

            return { RetCode::UnknownError, BTreeP{} };
        }


        /** Update item uid and mark the item as MRU

        @param [in] old_uid - obsolete uid
        @param [in] new_uid - actual uid
        @retval RetCode - operation status
        @throw nothing
        */
        [[nodiscard]]
        auto update_uid( NodeUid old_uid, NodeUid new_uid ) noexcept
        {
            using namespace std;

            try
            {
                shared_lock s{ mru_mutex_ };

                if ( auto item_it = mru_items_.find( old_uid ); item_it != end( mru_items_ ) )
                {
                    exclusive_lock e{ s };

                    // update uid and mark as MRU
                    auto item = mru_items_.extract( item_it );
                    auto order_it = item.mapped().second;
                    item.key() = new_uid;
                    *order_it = new_uid;
                    mru_items_.insert( move( item ) );
                    mru_order_.splice( end( mru_order_ ), mru_order_, order_it );
                }

                return RetCode::Ok;
            }
            catch ( ... )
            {
            }

            return RetCode::UnknownError;
        }


        auto drop( NodeUid uid ) noexcept
        {
            using namespace std;

            try
            {
                shared_lock s{ mru_mutex_ };

                if ( auto item_it = mru_items_.find( uid ); item_it != end( mru_items_ ) )
                {
                    exclusive_lock{ s };

                    // remove item from cache and free order slot
                    auto order_it = item_it->second.second;
                    mru_items_.erase( item_it );
                    *order_it = InvalidNodeUid;
                    mru_order_.splice( begin( mru_order_ ), mru_order_, order_it );
                }

                return RetCode::Ok;
            }
            catch ( ... )
            {
            }

            return RetCode::UnknownError;
        }
    };
}

#include "storage_file.h"
#include "b_tree.h"

#endif
