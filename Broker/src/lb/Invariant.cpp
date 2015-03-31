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
Invariant::Invariant(State state, float migrationStep, float migrationTotal, 
              std::map<std::string, float>& migrationReport, float generatorPower)
{
  m_state = state;
  m_MigrationStep = migrationStep;
  m_MigrationTotal = migrationTotal;
  m_MigrationReport = migrationReport;
  m_GeneratorPower = generatorPower;
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

  bool result = true;

  if(!CGlobalConfiguration::Instance().GetInvariantCheck())
  {
    Logger.Info << "Skipped invariant check, disabled." << std::endl;
  }
  else
  {
    float total_power_difference = m_MigrationTotal;

    BOOST_FOREACH(float power_difference, m_MigrationReport | boost::adaptors::map_values)
    {
      total_power_difference += power_difference;
    }

    Logger.Debug << "Invariant Variables:"
        << "\n\tEstimated Generator Power: " << m_GeneratorPower
        << "\n\tExpected Power Difference: " << total_power_difference
        << "\n\tMigration Step Size: " << m_MigrationStep
        << "\n\tMax Generator Power: " << GENERATOR_MAX_POWER << std::endl;

    if(m_State == LBAgent::SUPPLY)
    {
       Logger.Debug << "Checking the supply invariant." << std::endl;
       result &= m_GeneratorPower - total_power_difference >= m_MigrationStep;
    }
    if(m_State == LBAgent::DEMAND)
    {
      Logger.Debug << "Checking the demand invariant." << std::endl;
      result &= m_GeneratorPower - total_power_difference <= GENERATOR_MAX_POWER;
    }

    if(!result)
    {
      Logger.Info << "The physical invariant is false." << std::endl;
    }
  }

  return result;

}

}// broker
}// freedm

