////////////////////////////////////////////////////////////////////////////////
/// @file               invariantcheck.hpp
///
/// @author             Ravi Akella <rcaq5c@mst.edu>
/// @author             Thomas Roth <tprfh7@mst.edu>
/// @author             Daniel Grooms <dagn9c@mst.edu>
///
/// @project            FREEDM DGI
///
/// @description        An invariant function object for use in the load
///                     balance algorithm.
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


#ifndef INVARIANT_HPP
#define INVARIANT_HPP

#include "messages/ModuleMessage.pb.h"
#include "lb/LoadBalance.hpp"

#include <string>
#include <boost/shared_ptr.hpp>

namespace freedm {
namespace broker {

class Invariant
{
  private:
    typedef lb::lbAgent::State State;
    State m_State;
    float m_MigrationStep;
    float m_MigrationTotal;
    std::map<std::string, float> m_MigrationReport;
    float m_GeneratorPower;

  public:
    //Constructor
    Invariant(State state, float migrationStep, float migrationTotal, 
              std::map<std::string, float>& migrationReport, float generatorPower);

    //Overwrite () operator for quick function call in load balance.
    bool operator() ();
};

}//broker
}//freedm

#endif //invariantcheck.hpp

