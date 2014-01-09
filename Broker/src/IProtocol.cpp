////////////////////////////////////////////////////////////////////////////////
/// @file         IProtocol.cpp
///
/// @author       Derek Ditch <derek.ditch@mst.edu>
/// @author       Stephen Jackson <scj7t4@mst.edu>
///
/// @project      FREEDM DGI
///
/// @description  Declare CConnection class
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

#include "CConnection.hpp"
#include "CLogger.hpp"
#include "config.hpp"
#include "CReliableConnection.hpp"
#include "IProtocol.hpp"

#include <exception>

namespace freedm {
    namespace broker {

namespace {

/// This file's logger.
CLocalLogger Logger(__FILE__);

}

void IProtocol::Write(CMessage msg)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::array<char, CReliableConnection::MAX_PACKET_SIZE>::iterator it;

    /// Previously, we would call Synthesize here. Unfortunately, that was an
    /// Appalling heap of junk that didn't even work the way you expected it
    /// to. So, we're going to do something different.

    if(GetStopped())
        return;

    std::stringstream oss;
    std::string raw;
    /// Record the out message to the stream
    try
    {
        msg.Save(oss);
    }
    catch( std::exception &e )
    {
        Logger.Error<<"Couldn't write message to string stream."<<std::endl;
        throw;
    }
    raw = oss.str();
    /// Check to make sure it isn't goint to overfill our message packet:
    if(raw.length() > CReliableConnection::MAX_PACKET_SIZE)
    {
        Logger.Info << "Message too long for buffer" << std::endl;
        Logger.Info << raw << std::endl;
        throw std::runtime_error("Outgoing message is to long for buffer");
    }
    /// If that looks good, lets write it into our buffer.
    it = m_buffer.begin();
    /// Use std::copy to copy the string into the buffer starting at it.
    it = std::copy(raw.begin(),raw.end(),it);

    Logger.Debug<<"Writing "<<raw.length()<<" bytes to channel"<<std::endl;

    #ifdef CUSTOMNETWORK
    if((rand()%100) >= GetConnection()->GetReliability())
    {
        Logger.Info<<"Outgoing Packet Dropped ("<<GetConnection()->GetReliability()
                      <<") -> "<<GetConnection()->GetUUID()<<std::endl;
        return;
    }
    #endif
    // The length of the contents placed in the buffer should be the same length as
    // The string that was written into it.
    try
    {
        GetConnection()->GetSocket().send(boost::asio::buffer(m_buffer,raw.length()));
    }
    catch(boost::system::system_error &e)
    {
        Logger.Debug << "Writing Failed: " << e.what() << std::endl;
        GetConnection()->Stop();
    }
}

    }
}
