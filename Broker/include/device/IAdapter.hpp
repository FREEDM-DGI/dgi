////////////////////////////////////////////////////////////////////////////////
/// @file           IAdapter.hpp
///
/// @author         Thomas Roth <tprfh7@mst.edu>
/// @author         Michael Catanzaro <michael.catanzaro@mst.edu>
///
/// @project        FREEDM DGI
///
/// @description    Interface for a physical device adapter.
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

#ifndef I_ADAPTER_HPP
#define	I_ADAPTER_HPP

#include <set>
#include <string>
#include <utility>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace freedm {
namespace broker {
namespace device {

/// Type of the value for device signals.
typedef float SignalValue;

/// Type of the unique identifier for device values.
typedef std::pair<const std::string, const std::string> DeviceSignal;

/// Physical adapter device interface.
////////////////////////////////////////////////////////////////////////////////
/// Defines the interface each device uses to perform its operations.  The
/// concrete adapter is responsible for implementation of both Get and Set
/// functions.
///
/// @limitations None.
////////////////////////////////////////////////////////////////////////////////
class IAdapter
    : private boost::noncopyable
{
public:
    /// Pointer to a physical adapter.
    typedef boost::shared_ptr<IAdapter> Pointer;

    /// Starts the adapter.
    virtual void Start() = 0;
    
    /// Retrieves a value from a device.
    virtual SignalValue Get(const std::string device,
            const std::string signal) const = 0;

    /// Sets a value on a device.
    virtual void Set(const std::string device, const std::string signal,
            const SignalValue value) = 0;

    /// Virtual destructor for derived classes.
    virtual ~IAdapter();

    /// Register a device name with the adapter.
    void RegisterDevice(const std::string devid);
    
    /// Get the list of registered device names.
    std::set<std::string> GetDevices() const;
private:
    /// Set of registered device names.
    std::set<std::string> m_devices;
};

} // namespace device
} // namespace broker
} // namespace freedm

#endif // I_ADAPTER_HPP
