////////////////////////////////////////////////////////////////////////////////
/// @file         CBroker.cpp
///
/// @author       Derek Ditch <derek.ditch@mst.edu>
///
/// @project      FREEDM DGI
///
/// @description  Implements the CBroker class. This class implements the
///               "Broker" pattern from POSA1[1]. This implementation is modeled
///               after the Boost.Asio "http server 1" example[2].
///
/// @citations  [1] Frank Buschmann, Regine Meunier, Hans Rohnert, Peter
///                 Sommerlad, and Michael Stal. Pattern-Oriented Software
///                 Architecture Volume 1: A System of Patterns. Wiley, 1 ed,
///                 August 1996.
///
///             [2] Boost.Asio Examples
///     <http://www.boost.org/doc/libs/1_41_0/doc/html/boost_asio/examples.html>
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

#include "CBroker.hpp"
#include "CLogger.hpp"
#include "CGlobalPeerList.hpp"
#include "IPeerNode.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/foreach.hpp>

#include <map>

/// General FREEDM Namespace
namespace freedm {
    /// Broker Architecture Namespace
    namespace broker {

namespace {

/// This file's logger.
CLocalLogger Logger(__FILE__);

}
        
///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::CBroker
/// @description The constructor for the broker, providing the initial acceptor
/// @io provides and acceptor socket for incoming network connecitons.
/// @peers any node running the broker architecture.
/// @sharedmemory The dispatcher and connection manager are shared with the
///               modules.
/// @pre The port is free to be bound to.
/// @post An acceptor socket is bound on the freedm port awaiting connections
///       from other nodes.
/// @param p_address The address to bind the listening socket to.
/// @param p_port The port to bind the listening socket to.
/// @param p_dispatch The message dispatcher associated with this Broker
/// @param m_ios The ioservice used by this broker to perform socket operations
/// @param m_conMan The connection manager used by this broker.
/// @limitations Fails if the port is already in use.
///////////////////////////////////////////////////////////////////////////////
CBroker::CBroker(const std::string& p_address, const std::string& p_port,
    CDispatcher &p_dispatch, boost::asio::io_service &m_ios,
    freedm::broker::CConnectionManager &m_conMan)
    : m_ioService(m_ios),
      m_connManager(m_conMan),
      m_dispatch(p_dispatch),
      m_newConnection(new CListener(m_ioService, m_connManager, *this, m_conMan.GetUUID())),
      m_phasetimer(m_ios),
      m_synchronizer(*this),
      m_signals(m_ios,SIGINT)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::udp::resolver resolver(m_ioService);
    boost::asio::ip::udp::resolver::query query( p_address, p_port);
    boost::asio::ip::udp::endpoint endpoint = *resolver.resolve( query );
    
    // Listen for connections and create an event to spawn a new connection
    m_newConnection->GetSocket().open(endpoint.protocol());
    m_newConnection->GetSocket().bind(endpoint);
    m_connManager.Start(m_newConnection);
    m_busy = false;
    
    // Try to align on the first phase change
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    now += CGlobalConfiguration::instance().GetClockSkew();
    now -= boost::posix_time::milliseconds(2*ALIGNMENT_DURATION);
    m_last_alignment = now;

    m_phase = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::~CBroker
/// @description Cleans up all the timers from this module since the timers are
///     stored as pointers.
/// @pre None
/// @post All the timers are destroyed and their handles no longer point at
///     valid resources.
///////////////////////////////////////////////////////////////////////////////
CBroker::~CBroker()
{
    TimersMap::iterator it;
    for(it=m_timers.begin(); it!=m_timers.end(); it++)
    {
        delete (*it).second;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::Run()
/// @description Calls the ioservice run (initializing the ioservice thread)
///               and then blocks until the ioservice runs out of work.
/// @pre  The ioservice has not been allocated a thread to operate on and has
///       some schedule of jobs waiting to be performed (so it doesn't exit
///       immediately.)
/// @post The ioservice has terminated.
/// @return none
///////////////////////////////////////////////////////////////////////////////
void CBroker::Run()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    // The io_service::run() call will block until all asynchronous operations
    // have finished. While the server is running, there is always at least one
    // asynchronous operation outstanding: the asynchronous accept call waiting
    // for new incoming connections.
    m_signals.async_wait(boost::bind(&CBroker::HandleSignal, this,_1,_2));
    m_synchronizer.Run();
    m_ioService.run();
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::GetIOService
/// @description returns a refernce to the ioservice used by the broker.
/// @return The ioservice used by this broker.
///////////////////////////////////////////////////////////////////////////////
boost::asio::io_service& CBroker::GetIOService()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    return m_ioService;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::Stop
/// @description  Registers a stop command into the io_service's job queue.
///               when scheduled, the stop operation will terminate all running
///               modules and cause the ioservice.run() command to exit.
/// @pre The ioservice is running and processing tasks.
/// @post The command to stop the ioservice has been placed in the service's
///        task queue.
///////////////////////////////////////////////////////////////////////////////
void CBroker::Stop()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    // Post a call to the stop function so that CBroker::stop() is safe to call
    // from any thread.
    m_synchronizer.Stop();
    m_ioService.post(boost::bind(&CBroker::HandleStop, this));
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::HandleSignal
/// @description Handle signals from signal
/// @pre None
/// @post The broker winds down
///////////////////////////////////////////////////////////////////////////////
void CBroker::HandleSignal(const boost::system::error_code& error, int parameter)
{
    if(!error)
    {
        Logger.Fatal<<"Caught signal "<<parameter<<". Shutting Down..."<<std::endl;
        Stop();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::HandleStop
/// @description Handles closing all the sockets connection managers and
///              Services.
/// @pre The ioservice is running.
/// @post The ioservice is stopped.
///////////////////////////////////////////////////////////////////////////////
void CBroker::HandleStop()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    // The server is stopped by canceling all outstanding asynchronous
    // operations. Once all operations have finished the io_service::run() call
    // will exit.
    m_connManager.StopAll();
    m_ioService.stop(); 
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::RegisterModule
/// @description Places the module in to the list of schedulable phases. The
///   scheduler cycles through these in order to do real-time round robin
///   scheduling.
/// @pre None
/// @post The module is registered with a phase duration specified by the
///   parameter phase.
/// @param m the identifier for the module.
/// @param phase the duration of the phase.
///////////////////////////////////////////////////////////////////////////////
void CBroker::RegisterModule(CBroker::ModuleIdent m, boost::posix_time::time_duration phase)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::mutex::scoped_lock schlock(m_schmutex);
    boost::system::error_code err;
    bool exists = false;
    for(unsigned int i=0; i < m_modules.size(); i++)
    {
        if(m_modules[i].first == m)
        {
            exists = true;
            break;
        } 
    }
    if(!exists)
    {
        m_modules.push_back(PhaseTuple(m,phase));
        if(m_modules.size() == 1)
        {
            schlock.unlock();
            ChangePhase(err);
            schlock.lock();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::AllocateTimer
/// @description Returns a handle to a timer to use for scheduling tasks.
///     timer recycling helps prevent forest fires (and accidental branching
/// @pre The module is registered
/// @post A handle to a timer is returned.
/// @param module the module the timer should be allocated to
///////////////////////////////////////////////////////////////////////////////
CBroker::TimerHandle CBroker::AllocateTimer(CBroker::ModuleIdent module)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    
    boost::mutex::scoped_lock schlock(m_schmutex);
    CBroker::TimerHandle myhandle;
    boost::asio::deadline_timer* t = new boost::asio::deadline_timer(m_ioService);
    myhandle = m_handlercounter;
    m_handlercounter++;
    m_allocs.insert(CBroker::TimerAlloc::value_type(myhandle,module));
    m_timers.insert(CBroker::TimersMap::value_type(myhandle,t));
    m_nexttime.insert(CBroker::NextTimeMap::value_type(myhandle,false));
    m_ntexpired.insert(CBroker::NextTimeMap::value_type(myhandle,false));
    return myhandle;
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::Schedule
/// @description Given a binding to a function that should be run into the
///   future, prepares it to be run... in the future.
/// @param h The handle to the timer being set.
/// @param wait the amount of the time to wait. If this value is "not_a_date_time"
///     The wait is converted to positive infinity and the time will expire as 
///     soon as the module no longer owns the context.
/// @param x The partially bound function that will be scheduled.
/// @pre The module is registered
/// @post A function is scheduled to be called in the future. If a next time
///     function is scheduled, its timer will expire as soon as its round ends.
///////////////////////////////////////////////////////////////////////////////
void CBroker::Schedule(CBroker::TimerHandle h,
    boost::posix_time::time_duration wait, CBroker::Scheduleable x)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::mutex::scoped_lock schlock(m_schmutex);
    CBroker::Scheduleable s;
    if(wait.is_not_a_date_time())
    {
        wait = boost::posix_time::time_duration(boost::posix_time::pos_infin);
        m_nexttime[h] = true;
    }
    else
    {
        m_nexttime[h] = false;
    }
    m_timers[h]->expires_from_now(wait);
    s = boost::bind(&CBroker::ScheduledTask,this,x,h,boost::asio::placeholders::error);
    Logger.Debug<<"Scheduled task for timer "<<h<<std::endl;
    m_timers[h]->async_wait(s);
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::Schedule
/// @description Given a module and a bound schedulable, enter that schedulable
///     into that modules job queue.
/// @pre The module is registered.
/// @post The task is placed in the work queue for the module m. If the
///     start_worker parameter is set to true, the module's worker will be
///     activated if it isn't already.
/// @param m The module the schedulable should be run as.
/// @param x The method that will be run.
/// @param start_worker tells the worker to begin processing again, if it is
///     currently idle [The worker will be idle if the work queue is empty; this
///     can be useful to defer an activity to the next round if the node is not busy
///////////////////////////////////////////////////////////////////////////////
void CBroker::Schedule(ModuleIdent m, BoundScheduleable x, bool start_worker)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::mutex::scoped_lock schlock(m_schmutex);
    m_ready[m].push_back(x);
    if(!m_busy && start_worker)
    {
        schlock.unlock();
        Worker();
        schlock.lock();
    }
    Logger.Debug<<"Module "<<m<<" now has queue size: "<<m_ready[m].size()<<std::endl;
    Logger.Debug<<"Scheduled task (NODELAY) for "<<m<<std::endl;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::ChangePhase
/// @description This task will mark to the schedule that it is time to change
///     phases. This will change which functions will be put into the queue
/// @pre None
/// @post The phase has been changed.
///////////////////////////////////////////////////////////////////////////////
void CBroker::ChangePhase(const boost::system::error_code &err)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    if(m_modules.size() == 0)
    {
        m_phase=0;
        return;
    }
    unsigned int oldphase = m_phase;
    // Past this point assume there is at least one module.
    boost::mutex::scoped_lock schlock(m_schmutex);
    m_phase++;
    // Get the time without millisec and with millisec then see how many millsec we
    // are into this second.
    // Generate a clock beacon
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration time = now.time_of_day();
    time += CGlobalConfiguration::instance().GetClockSkew();

    if(m_phase >= m_modules.size())
    {
        m_phase = 0;
    }
    unsigned int round = 0;
    for(unsigned int i=0; i < m_modules.size(); i++)
    {
        round += m_modules[i].second.total_milliseconds();
    }
    unsigned int millisecs = time.total_milliseconds();
    unsigned int intoround = (millisecs % round);
    unsigned int cphase = 0;
    unsigned int tmp = m_modules[0].second.total_milliseconds();
    // Pre: Assume it should be the first phase.
    // Step: Consider how long the phase would be if it ran in its entirety. If
    //  completing that phase would go beyod the amount of time in the
    //  round so far (considering all the time that would be used by other phases up
    //  to that point) then that phase is the current one.
    // Post: CPhase should be the current phase and tmp should be 
    while(cphase < m_modules.size() && tmp < intoround)
    {
        cphase++;
        tmp += m_modules[cphase].second.total_milliseconds();
    }
    unsigned int remaining = tmp-intoround;
    unsigned int sched_duration = m_modules[m_phase].second.total_milliseconds();
    // How we want to do this is that every so of tone we want to figure out
    // what phase it should be and then schedule that phase?
    // As an aside, you could tune alignment duration down to 0 so that every
    // phase is specifically assigned to a time slice.
    if(now-m_last_alignment > boost::posix_time::milliseconds(ALIGNMENT_DURATION))
    {
        Logger.Notice<<"Aligned phase to "<<cphase<<" (was "<<m_phase<<") for "
                   <<remaining<<" ms"<<std::endl;

        
        m_phase = cphase;
        m_last_alignment = now;
        sched_duration = remaining;
    }
    if(m_modules.size() > 0)
    {
        Logger.Notice<<"Phase: "<<m_modules[m_phase].first<<" for "<<sched_duration<<"ms "<<"offset "<<CGlobalConfiguration::instance().GetClockSkew()<<std::endl;
    }
    if(m_phase != oldphase)
    {
        m_connManager.ChangePhase((m_phase==0));
        std::string oldident = m_modules[oldphase].first;
        Logger.Notice<<"Changed Phase: expiring next time timers for "<<oldident<<std::endl;
        // Look through the timers for the module and see if any of them are
        // set for next time:
        BOOST_FOREACH(CBroker::TimerAlloc::value_type t, m_allocs)
        {
            Logger.Debug<<"Examine timer "<<t.first<<" for module "<<t.second<<" expire nexttime: "
                        <<m_nexttime[t.first]<<std::endl;
            if(t.second == oldident && m_nexttime[t.first] == true)
            {
                Logger.Notice<<"Scheduling task for next time timer: "<<t.first<<std::endl;
                m_timers[t.first]->cancel();                
                m_nexttime[t.first] = false;
                m_ntexpired[t.first] = true;
            }   
        }
    }
    //If the worker isn't going, start him again when you change phases.
    if(!m_busy)
    {
        schlock.unlock();
        Worker();
        schlock.lock();
    }
    boost::posix_time::time_duration r = boost::posix_time::milliseconds(sched_duration);
    m_phaseends = now + r;
    m_phasetimer.expires_from_now(r);
    m_phasetimer.async_wait(boost::bind(&CBroker::ChangePhase,this,
        boost::asio::placeholders::error));
}
#pragma GCC diagnostic warning "-Wunused-parameter"

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::TimeRemaining
/// @description Shows how much time is remaining in the current pgase
/// @pre The Change Phase function has been called at least once. This should
///     have occured by the time the first module is ready to look at the 
///     remaining time.
/// @post no change
/// @return A time_duration describing the amount of time remaining in the
///     phase.
///////////////////////////////////////////////////////////////////////////////
boost::posix_time::time_duration CBroker::TimeRemaining()
{
    return m_phaseends - boost::posix_time::microsec_clock::universal_time();
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::ScheduledTask
/// @description When a timer for a task expires, it enters this phase. The
///     timer is removed from the timers list. Then Execute is called to keep
///     the work queue going.
/// @pre A task is scheduled for execution
/// @post The task is entered into th ready queue. 
///////////////////////////////////////////////////////////////////////////////
void CBroker::ScheduledTask(CBroker::Scheduleable x, CBroker::TimerHandle handle,
    const boost::system::error_code &err)
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::mutex::scoped_lock schlock(m_schmutex);
    ModuleIdent module = m_allocs[handle];
    boost::system::error_code serr;
    if(m_ntexpired[handle])
    {
        m_ntexpired[handle] = false;
    }
    else
    {
        serr = err;
    }
    Logger.Debug<<"Handle finished: "<<handle<<" For module "<<module<<std::endl;
    // First, prepare another bind, which uses the given error
    CBroker::BoundScheduleable y = boost::bind(x,serr);
    // Put it into the ready queue
    m_ready[module].push_back(y);
    Logger.Debug<<"Module "<<module<<" now has queue size: "<<m_ready[module].size()<<std::endl;
    if(!m_busy)
    {
        schlock.unlock();
        Worker();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @fn CBroker::Worker
/// @description Reads the current phase and if the phase is correct, queues
///     all the tasks for that phase to the ioservice. If m_busy is set, the 
///     worker is still working on clearing the queue. If it's set to false,
///     the worker needs to be started when the scheduled task is called
/// @pre None
/// @post A task is scheduled to run.
///////////////////////////////////////////////////////////////////////////////
void CBroker::Worker()
{
    Logger.Trace << __PRETTY_FUNCTION__ << std::endl;
    boost::mutex::scoped_lock schlock(m_schmutex);
    if(m_phase >= m_modules.size())
    {
        m_busy = false;
        schlock.unlock();
        return;
    }
    std::string active = m_modules[m_phase].first;
    if(m_ready[active].size() > 0)
    {
        Logger.Debug<<"Performing Job"<<std::endl;
        // Mark that the worker has something to do
        m_busy = true;
        // Extract the first item from the work queue:
        CBroker::BoundScheduleable x = m_ready[active].front();
        m_ready[active].pop_front();
        // Execute the task.
        schlock.unlock();
        x();
        schlock.lock();
    }
    else
    {
        m_busy = false;
        schlock.unlock();
        return;
    }
    // Schedule the worker again:
    m_ioService.post(boost::bind(&CBroker::Worker, this));
}

//////////////////////////////////
/// Returns the clock synchronizer
//////////////////////////////////
CClockSynchronizer& CBroker::GetClockSynchronizer()
{
    return m_synchronizer;
}

    } // namespace broker
} // namespace freedm
