////////////////////////////////////////////////////////////////////////////////
/// @file           FreedmExceptions.hpp
///
/// @author         Michael Catanzaro <michael.catanzaro@mst.edu>
///
/// @project        FREEDM DGI
///
/// @description    Declarations of general FREEDM exception classes.
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

#ifndef FREEDM_EXCEPTIONS_HPP
#define FREEDM_EXCEPTIONS_HPP

#include <boost/exception/all.hpp>

namespace freedm {
namespace broker {

/// Type of an informative error message, replacment for std::exception::what
typedef boost::error_info<struct tag_what, std::string> what;

/// Noninstantiable base class of the FREEDM exception hierarchy
struct IFreedmException
    : virtual std::exception
    , virtual boost::exception
{
    ///////////////////////////////////////////////////////////////////////////
    /// The noninstantiable base class of the FREEDM exception hierarchy. All
    /// developer-defined exceptions should inherit virtually from this class.
    /// Note that this class does NOT provide a virtual destructor, so
    /// subclasses may not have any data members. To add arbitrary information
    /// to this exception, use this class's insertion operator (inherited from
    /// boost::exception) to add arbitrary boost::error_info instances. See the
    /// Boost Exception documentation for more details. Don't forget to throw
    /// with the BOOST_THROW_EXCEPTION macro to add info about the site of the
    /// throw! When rethrowing an exception, use a standard throw statement
    /// without specifying an exception to throw in order to rethrow the
    /// currently-handled exception; attempting to specify an exception to
    /// rethrow will fail because this class provides no copy constructor (to
    /// avoid slicing off the type of the exception.)
    ///////////////////////////////////////////////////////////////////////////
protected:
    /// Protected constructor to prevent direct instantiation
    IFreedmException() { }
};

/// Exception type for errors related to network connections.
struct EConnectionError : virtual IFreedmException
{
    virtual const char *what() const throw()
    {
        return "networking error";
    }
};

} // namespace freedm
} // namespace broker

#endif // FREEDM_EXCEPTIONS_HPP
