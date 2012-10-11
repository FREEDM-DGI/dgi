////////////////////////////////////////////////////////////////////////////////
/// @file         CSRConnection.hpp
///
/// @author       Derek Ditch <derek.ditch@mst.edu>
/// @author       Stephen Jackson <scj7t4@mst.edu>
///
/// @project      FREEDM DGI
///
/// @description  Declare CSRConnection class
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

#ifndef CSRCONNECTION_HPP
#define CSRCONNECTION_HPP

#include "CConnection.hpp"
#include "CMessage.hpp"
#include "IProtocol.hpp"

#include <deque>
#include <iomanip>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace freedm {
    namespace broker {


/// A reliable connection protocol with sweet as expirations.
class CSRConnection : public IProtocol
    , public boost::enable_shared_from_this<CSRConnection>
{
    public:
        /// Initializes the protocol with the underlying connection
        CSRConnection(CConnection * conn);
        /// Destructor
        virtual ~CSRConnection() { };
        /// Public facing send function that sends a message
        void Send(CMessage msg);
        /// Public facing function that handles marking down ACKs for sent messages
        void RecieveACK(const CMessage &msg);
        /// deterimines if a  messageshould be given to the dispatcher
        bool Recieve(const CMessage &msg);
        /// Handles Writing an ack for the input message to the channel
        void SendACK(const CMessage &msg);
        /// Sends a synchronizer
        void SendSYN();
        /// Stops the timers
        void Stop() { m_timeout.cancel(); SetStopped(true); };
        /// Returns the identifier
        std::string GetIdentifier() { return Identifier(); };
        /// Returns the identifier for this protocol.
        static std::string Identifier() { return "SRC"; };
    private:
        /// Resend outstanding messages
        void Resend(const boost::system::error_code& err);
        /// Timeout for resends
        boost::asio::deadline_timer m_timeout;
        /// The current ack to flood with
        CMessage m_currentack;
        /// The expected next in sequence number
        unsigned int m_inseq;
        /// The next number to assign to an outgoing message
        unsigned int m_outseq;
        /// Marks if this has been synced.
        bool m_insync;
        /// Counts the number of times this one has been resynced
        unsigned int m_inresyncs;
        /// Time the last accepted sync was
        boost::posix_time::ptime m_insynctime;
        /// Marks if we've sent the outsync for this connection
        bool m_outsync;
        /// Keeps track of the last resync that we've seen
        boost::posix_time::ptime m_outsynctime;
        /// Marks if we should send the kill hash.
        bool m_sendkills;
        /// The hash to... MURDER.
        unsigned int  m_sendkill;
        /// The window
        std::deque<CMessage> m_window;
        /// Sequence modulo
        static const unsigned int SEQUENCE_MODULO = 1024;
        /// Refire time in MS
        static const unsigned int REFIRE_TIME = 10;
        /// Mutex for the current ack
        boost::shared_mutex m_ackmutex;
};

    }
}
#endif
