////////////////////////////////////////////////////////////////////////////////
/// @file         CUuid.hpp
///
/// @project      FREEDM DGI
///
/// @description  Used for initial generation of UUIDs.
///
/// These source code files were created at Missouri University of Science and
/// Technology, and are intended for use in teaching or research. They may be
/// freely copied, modified, and redistributed as long as modified versions are
/// clearly marked as such and this notice is not removed. Neither the authors
/// nor Missouri S&T make any warranty, express or implied, nor assume any legal
/// responsibility for the accuracy, completeness, or usefulness of these files
/// or any information distributed with these files.
///
/// Suggested modifications or questions about these files can be directed to
/// Dr. Bruce McMillin, Department of Computer Science, Missouri University of
/// Science and Technology, Rolla, MO 65409 <ff@mst.edu>.
////////////////////////////////////////////////////////////////////////////////

#ifndef FREEDM_UUID_HPP
#define FREEDM_UUID_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace freedm {
namespace broker {

/// A rarely used type for uuids, used for initial generation of uuids
class CUuid : public boost::uuids::uuid
{
public:
    /// Intializes the uuid and the uuid generator
    CUuid()
        : boost::uuids::uuid(boost::uuids::random_generator()())
    {}

    /// Copy constructor for the UUID generator
    explicit CUuid(boost::uuids::uuid const& u)
        : boost::uuids::uuid(u)
    {}

    /// Returns a UUID in the DNS namespace for the given hostname.
    static CUuid from_dns( const std::string &s, const std::string &p )
	{
    	boost::uuids::uuid dns_namespace =
    		boost::uuids::string_generator()(
    				"{6ba7b810-9dad-11d1-80b4-00c04fd430c8}"
    		);
    	return CUuid(boost::uuids::name_generator(dns_namespace)(s+":"+p));
	}
};

} // namespace broker
} // namespace freedm

#endif // FREEDM_UUID_HPP
