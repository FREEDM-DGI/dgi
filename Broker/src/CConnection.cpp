////////////////////////////////////////////////////////////////////////////////
/// @file         CConnection.cpp
///
/// @author       Derek Ditch <derek.ditch@mst.edu>
/// @author       Christopher M. Kohlhoff <chris@kohlhoff.com> (Boost Example)
/// @author       Stephen Jackson <scj7t4@mst.edu>
///
/// @project      FREEDM DGI
///
/// @description  Represents a single outgoing connection to a client.
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
#include "CConnectionManager.hpp"
#include "CDispatcher.hpp"
#include "CLogger.hpp"
#include "CMessage.hpp"
#include "config.hpp"
#include "CSRConnection.hpp"
#include "CSUConnection.hpp"
#include "CSRSWConnection.hpp"
#include "RequestParser.hpp"

#include <vector>

#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

namespace freedm {
namespace broker {
        
namespace {

/// This file's logger.
CLocalLogger Logger(__FILE__);

}
        
///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::CConnection
/// @description Constructor for the CConnection object. Since the change to
///   udp, this object can act either as a listener or sender (but not both)
///   to have the object behave as a listener, Start() should be called on it.
/// @pre An initialized socket is ready to be converted to a connection.
/// @post A new CConnection object is initialized.
/// @param p_ioService The socket to use for the connection.
/// @param p_manager The related connection manager that tracks this object.
/// @param p_broker The broker used for scheduling the message deliveries
/// @param uuid The uuid this node connects to, or what listener.
///////////////////////////////////////////////////////////////////////////////
CConnection::CConnection(boost::asio::io_service& p_ioService,
  CConnectionManager& p_manager, CBroker& p_broker, std::string uuid)
  : CReliableConnection(p_ioService,p_manager,p_broker,uuid)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    m_protocols.insert(ProtocolMap::value_type(CSUConnection::Identifier(),
        ProtocolPtr(new CSUConnection(this))));
    m_protocols.insert(ProtocolMap::value_type(CSRConnection::Identifier(),
        ProtocolPtr(new CSRConnection(this))));
    m_protocols.insert(ProtocolMap::value_type(CSRSWConnection::Identifier(),
        ProtocolPtr(new CSRSWConnection(this))));
    m_defaultprotocol = CSRConnection::Identifier();
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::Start
/// @description Starts the recieve routine which causes this socket to behave
///   as a listener.
/// @pre The object is initialized.
/// @post The connection is asynchronously waiting for messages.
///////////////////////////////////////////////////////////////////////////////
void CConnection::Start()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::Stop
/// @description Stops the socket and cancels the timeout timer. Does not
///   need to be called on a listening connection (ie one that has had
///   Start() called on it.
/// @pre Any initialized CConnection object.
/// @post The underlying socket is closed and the message timeout timer is
///        cancelled.
///////////////////////////////////////////////////////////////////////////////
void CConnection::Stop()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    ProtocolMap::iterator sit;
    for(sit = m_protocols.begin(); sit != m_protocols.end(); sit++)
    {
        (*sit).second->Stop();
    }   
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    GetSocket().close();
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::ChangePhase
/// @description An event that gets called when the broker changes the current
///   phase.
/// @pre None
/// @post The sliding window protocol if it exists is stopped.
/// @param newround If true, the phase change is also the start of an entirely
///     new round.
///////////////////////////////////////////////////////////////////////////////
void CConnection::ChangePhase(bool newround)
{
    ProtocolMap::iterator it;
    for(it = m_protocols.begin(); it != m_protocols.end(); it++)
    {
        it->second->ChangePhase(newround);
    }
}
 
///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::Send
/// @description Given a message and wether or not it should be sequenced,
///   write that message to the channel.
/// @pre The CConnection object is initialized.
/// @post If the window is in not full, the message will have been written to
///   to the channel. Before being sent the message has been signed with the
///   UUID, source hostname and sequence number (if it is being sequenced).
///   If the message is being sequenced  and the window is not already full,
///   the timeout timer is cancelled and reset.
/// @param p_mesg A CMessage to write to the channel.
///////////////////////////////////////////////////////////////////////////////
void CConnection::Send(CMessage & p_mesg)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;

    // If the UUID of the reciepient (The value stored by GetUUID of this
    // object) is the same as the this node's uuid (As stored by the
    // Connection manager) place the message directly into the recieved
    // Queue.
    if(GetUUID() == GetConnectionManager().GetUUID())
    {
        p_mesg.SetSourceUUID(GetConnectionManager().GetUUID());
        p_mesg.SetSourceHostname(GetConnectionManager().GetHostname());
        p_mesg.SetSendTimestampNow();
        MessagePtr local(new CMessage);
        *local = p_mesg;
        GetDispatcher().HandleRequest(GetBroker(),local);
        return;
    }

    ProtocolMap::iterator sit = m_protocols.find(p_mesg.GetProtocol());    
    
    if(sit == m_protocols.end())
    {
        sit = m_protocols.find(m_defaultprotocol);
    }
    (*sit).second->Send(p_mesg);
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::RecieveACK
/// @description Handler for recieving acknowledgments from a sender.
/// @pre Initialized connection.
/// @post The message with sequence number has been acknowledged and all
///   messages sent before that message have been considered acknowledged as
///   well.
/// @param msg The message to consider as acknnowledged
///////////////////////////////////////////////////////////////////////////////
void CConnection::RecieveACK(const CMessage &msg)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    std::string protocol = msg.GetProtocol();
    ProtocolMap::iterator sit = m_protocols.find(protocol);
    if(sit != m_protocols.end())
    {
        (*sit).second->RecieveACK(msg);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CConnection::Recieve
/// @description Handler for determineing if a recieved message should be ACKd
/// @pre Initialized connection.
/// @post The message with sequence number has been acknowledged and all
///   messages sent before that message have been considered acknowledged as
///   well.
/// @param msg The message to consider as acknnowledged
///////////////////////////////////////////////////////////////////////////////
bool CConnection::Recieve(const CMessage &msg)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    ProtocolMap::iterator sit = m_protocols.find(msg.GetProtocol());
    if(sit != m_protocols.end())
    {
        bool x = (*sit).second->Recieve(msg);
        if(x)
        {
            (*sit).second->SendACK(msg);
            return true;
        }
    }
    return false;
}

    } // namespace broker
} // namespace freedm
