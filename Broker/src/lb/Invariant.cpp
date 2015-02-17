////////////////////////////////////////////////////////////////////////////////
/// @file               invariantcheck.hpp
///
/// @author             Ravi Akella <rcaq5c@mst.edu>
/// @author             Thomas Roth <tprfh7@mst.edu>
/// @author             Daniel Grooms <dagn9c@mst.edu>
///
/// @project            FREEDM DGI
///
/// @description        An implementation file for the invariantcheck class.
///
/// @functions          Invariant::Invariant
///                     Invariant::operator()
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


#include <boost/foreach.hpp>

namespace freedm {
namespace broker {

namespace {
CLocalLogger Logger(__FILE__);
}

// Constructor, handles all invariant input variables for a
// invariant() functor call with fewest possible parameters.
Invariant::Invariant()
{
// Will store constructor values for custom invariants
}

///////////////////////////////////////////////////////////////////////////////
/// operator()
/// @description Evaluates the current truth of the physical invariant
/// @pre none
/// @post calculate the physical invariant using the Omega device
/// @return the truth value of the physical invariant
///////////////////////////////////////////////////////////////////////////////

bool Invariant::operator()()
{
  Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

  const float OMEGA_STEADY_STATE = 376.8;
  const int SCALING_FACTOR = 1000;

  bool result = true;
  std::set<device::CDevice::Pointer> container;
  container = device::CDeviceManager::Instance().GetDevicesOfType("Omega");

  if(container.size() > 0 && CGlobalConfiguration::Instance().GetInvariantCheck())
  {
    if(container.size() > 1)
    {
      Logger.Warn << "Multiple attached frequency devices." << std::endl;
    }
    float w  = (*container.begin())->GetState("frequency");
    float P  = SCALING_FACTOR * m_PowerDifferential;
    float dK = SCALING_FACTOR * (m_PowerDifferential + m_MigrationStep);
    float freq_diff = w - OMEGA_STEADY_STATE;

    Logger.Info << "Invariant Variables:"
                << "\n\tw  = " << w
                << "\n\tP  = " << P
                << "\n\tdK = " << dK << std::endl;

    result &= freq_diff*freq_diff*(0.1*w+0.008)+freq_diff*((5.001e-8)*P*P) > freq_diff*dK;

    if(!result)
    {
      Logger.Info << "The physical invariant is false." << std::endl;
    }

  }

  return result;
}

}// broker
}// freedm

