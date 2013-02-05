////////////////////////////////////////////////////////////////////////////////
/// @file         CDeviceLogger.cpp
///
/// @author       Thomas Roth <tprfh7@mst.edu>
///
/// @project      FREEDM DGI
///
/// @description  Represents a distributed energy storage device.
///
/// @functions
///     CDeviceLogger::CDeviceLogger
///     CDeviceLogger::~CDeviceLogger
///     CDeviceLogger::SetGroupStatus
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

#include "CDeviceLogger.hpp"
#include "CLogger.hpp"

namespace freedm {
namespace broker {
namespace device {

namespace {
/// This file's logger.
CLocalLogger Logger(__FILE__);
}

////////////////////////////////////////////////////////////////////////////////
/// Constructs the logger.
///
/// @pre None.
/// @post Constructs a new device.
/// @param device The unique identifier for the device.
/// @param adapter The adapter that implements operations for this device.
///
/// @limitations None.
////////////////////////////////////////////////////////////////////////////////
CDeviceLogger::CDeviceLogger(std::string device, IAdapter::Pointer adapter)
    : IDevice(device, adapter)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
/// Virtual destructor for derived classes.
///
/// @pre None.
/// @post Destructs the object.
///
/// @limitations None.
////////////////////////////////////////////////////////////////////////////////
CDeviceLogger::~CDeviceLogger()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
/// Sets the current group membership status.
///
/// @pre None.
/// @post Calls IAdapter::Set with the signal "groupStatus".
/// @param status The new floating point group status.
///
/// @limitations The group status must be represented as a floating point.
////////////////////////////////////////////////////////////////////////////////
void CDeviceLogger::SetGroupStatus(const SignalValue status)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    Set("groupStatus", status);
    Logger.Info << "Set group status: " << status << std::endl;
}

} // namespace device
} // namespace broker
} // namespace freedm
