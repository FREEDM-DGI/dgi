////////////////////////////////////////////////////////////////////////////////
/// @file               ReversePowerFlowInvariant.cpp
///
/// @author             Thomas Roth <tprfh7@mst.edu>
/// @author             Daniel Grooms <dagn9c@mst.edu>
///
/// @project            FREEDM DGI
///
/// @description        An implementation file for the invariantcheck class.
///
/// @functions          ReversePowerFlowInvariant::ReversePowerFlowInvariant
///                     ReversePowerFlowInvariant::operator()
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

///////////////////////////////////////////////////////////////////////////////
/// ReversePowerFlowInvariant
/// @description: Constructor for the ReversePowerFlowInvariant
/// @pre: None
/// @post: The value of the invariant result is calculated
/// @param: state The load balance state
/// @param: migrationStep The step of the migration in load balance
/// @param: migrationTotal The migration total in load balance
/// @param: migrationReport The migration report in load balance
/// @param: generatorPower The power of the generator from load balance
///////////////////////////////////////////////////////////////////////////////
ReversePowerFlowInvariant::ReversePowerFlowInvariant(State state, float migrationStep,
        float migrationTotal, std::map<std::string, float>& migrationReport, float generatorPower)
{
    m_state = state;
    m_MigrationStep = migrationStep;
    m_MigrationTotal = migrationTotal;
    m_MigrationReport = migrationReport;
    m_GeneratorPower = generatorPower;
    m_result = true;

    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

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
            m_result &= m_GeneratorPower - total_power_difference >= m_MigrationStep;
        }
        if(m_State == LBAgent::DEMAND)
        {
            Logger.Debug << "Checking the demand invariant." << std::endl;
            m_result &= m_GeneratorPower - total_power_difference <= \
                        GENERATOR_MAX_POWER - m_MigrationStep;
        }

        if(!m_result)
        {
            Logger.Info << "The physical invariant is false." << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// operator()
/// @description Evaluates the current truth of the physical invariant
/// @pre None
/// @post Return the value of the physical invariant using the Omega device
/// @return The truth value of the physical invariant
///////////////////////////////////////////////////////////////////////////////
bool Invariant::operator()()
{
    return m_result;
}

}// broker
}// freedm

