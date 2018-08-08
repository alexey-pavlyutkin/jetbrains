#ifndef __JB__PHYSICAL_VOLUME__H__
#define __JB__PHYSICAL_VOLUME__H__


#include <memory>


namespace jb
{

    template < typename Policies, typename Pad > class Storage;
    template < typename Policies, typename Pad > class VirtualVolumeImpl;
    template < typename Policies, typename Pad > class MountPointImpl;
    template < typename Policies, typename Pad > class PhysicalVolumeImpl;


    template < typename Policies, typename Pad >
    class PhysicalVolume
    {
        friend typename Pad;
        template < typename T > friend struct Hash;

        using Storage            = ::jb::Storage< Policies, Pad >;
        using VirtualVolumeImpl  = ::jb::VirtualVolumeImpl< Policies, Pad >;
        using MountPointImpl     = ::jb::MountPointImpl< Policies, Pad >;
        using Impl               = ::jb::PhysicalVolumeImpl< Policies, Pad >;

        friend class Storage;
        friend class VirtualVolumeImpl;
        friend class MountPointImpl;

        using KeyCharT = typename Policies::KeyCharT;
        using KeyValueT = typename Policies::KeyValueT;
        using KeyRefT = typename Policies::KeyPolicy::KeyRefT;
        using KeyHashT = typename Policies::KeyPolicy::KeyHashT;
        using KeyHashF = typename Policies::KeyPolicy::KeyHashF;

        static constexpr size_t MountPointLimit = Policies::VirtualVolumePolicy::MountPointLimit;
        
        std::weak_ptr< Impl > impl_;

        PhysicalVolume( const std::shared_ptr< Impl > & impl ) noexcept :impl_( impl ) {}


    public:



        auto get_mount( KeyRefT path )
        {
            return std::pair{ RetCode::Ok, std::shared_ptr< MountPointImpl >() };
        }

        PhysicalVolume( ) noexcept = default;

        PhysicalVolume( const PhysicalVolume & o ) noexcept = default;

        PhysicalVolume( PhysicalVolume && o ) noexcept = default;

        PhysicalVolume & operator = ( const PhysicalVolume & o ) noexcept = default;

        PhysicalVolume & operator = ( PhysicalVolume && o ) noexcept = default;

        operator bool( ) const noexcept { return impl_.lock( );  }

        friend bool operator == ( const PhysicalVolume & l, const PhysicalVolume & r ) noexcept
        {
            return l.impl_.lock( ) == r.impl_.lock( );
        }

        friend bool operator != ( const PhysicalVolume & l, const PhysicalVolume & r ) noexcept
        {
            return l.impl_.lock( ) != r.impl_.lock( );
        }

        RetCode Close( ) const noexcept
        {
            Storage::close( *this );
        }
 
        RetCode PrioritizeOnTop( ) const noexcept
        {
            return Storage::prioritize_on_top( *this );
        }

        RetCode PrioritizeOnBottom( ) const noexcept
        {
            return Storage::prioritize_on_bottom( *this );
        }

        RetCode PrioritizeBefore( const PhysicalVolume & before ) const noexcept
        {
            return Storage::prioritize_on_top( *this, before );
        }

        RetCode PrioritizeAfter( const PhysicalVolume & after ) const noexcept
        {
            return Storage::prioritize_on_top( *this, after );
        }
    };
}

#endif
