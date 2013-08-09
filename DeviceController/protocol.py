# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
###############################################################################
# @file           protocol.py
#
# @author         Michael Catanzaro <michael.catanzaro@mst.edu>
#
# @project        FREEDM Device Controller
#
# @description    Logic for participating in the plug and play protocol.
#
# These source code files were created at Missouri University of Science and
# Technology, and are intended for use in teaching or research. They may be
# freely copied, modified, and redistributed as long as modified versions are
# clearly marked as such and this notice is not removed. Neither the authors
# nor Missouri S&T make any warranty, express or implied, nor assume any legal
# responsibility for the accuracy, completeness, or usefulness of these files
# or any information distributed with these files.
#
# Suggested modifications or questions about these files can be directed to
# Dr. Bruce McMillin, Department of Computer Science, Missouri University of
# Science and Technology, Rolla, MO 65409 <ff@mst.edu>.
###############################################################################

import socket
import sys
import time

from device import *

class ConnectionLostError(Exception):
    """
    An exception to be raised when the DGI disappears.
    """
    pass


def send_all(socket, msg):
    """
    Ensures the entirety of msg is sent over the socket.

    @param socket the connected stream socket to send to
    @param msg the message to be sent

    @ErrorHandling Could raise socket.error or socket.timeout if the data
                   cannot be sent in time. Raises ConnectionLostError if the
                   connection to the DGI is closed.
    """
    assert len(msg) != 0
    sent_bytes = socket.send(msg)
    while (sent_bytes != len(msg)):
        assert sent_bytes < len(msg)
        msg = msg[sent_bytes:]
        assert len(msg) != 0
        sent_bytes = socket.send(msg)
        if sent_bytes == 0:
            raise ConnectionLostError('Connection to DGI unexpectedly lost')


def recv_all(socket):
    """
    Receives all the data that has been sent from DGI until \r\n\r\n is
    reached.
    
    @param socket the connected stream socket to receive from

    @ErrorHandling Raises ConnectionLostError if the DGI times out, or
                   RuntimeError if a bad packet is detected.

    @return the data that has been read
    """
    msg = socket.recv(1024)
    if len(msg) == 0:
        raise ConnectionLostError('Connection to DGI unexpectedly lost')
    while not msg.endswith('\r\n\r\n'):
        if '\r\n\r\n' in msg:
            raise RuntimeError('DGI sent two messages in a row? :\n' + msg)
        new_msg = socket.recv(1024)
        if len(new_msg) == 0:
            raise ConnectionLostError('Connection to DGI unexpectedly lost')
        else:
            msg += new_msg
    return msg


def handle_bad_request(msg):
    """
    Terminate immediately if the DGI says we sent a bad request.

    In the future, we might want this function to be installable.

    @param msg string the message sent by DGI
    """
    msg = msg.replace('BadRequest', '', 1)
    raise RuntimeError('Sent bad request to DGI: ' + msg)


def send_states(adaptersock, devices):
    """
    Sends updated device states to the DGI.

    @param adaptersock the connected stream socket to send the states on
    @param devices set of devices attached to this controller

    @ErrorHandling caller must check for socket.timeout
    """
    msg = 'DeviceStates\r\n'
    for device in devices:
        for signal in device.get_signals():
            msg += '{0} {1} {2}\r\n'.format(device.get_name(),
                                            signal, 
                                            device.get_signal(signal))
    msg += '\r\n'
    print 'Sending states to DGI:\n' + msg.strip()
    send_all(adaptersock, msg)
    print 'Sent states to DGI\n'


def receive_commands(adaptersock, devices):
    """
    Receives new commands from the DGI and then implements them by modifying
    the devices map.

    @param adaptersock the connected stream socket to receive commands from
    @param devices set of devices attached to this controller

    @ErrorHandling caller must check for socket.timeout
    """
    print 'Awaiting commands from DGI...'
    msg = recv_all(adaptersock)
    print 'Received commands from DGI:\n' + msg.strip()

    if msg.find('BadRequest') == 0:
        handle_bad_request(msg)
    elif msg.find('Error') == 0:
        msg = msg.replace('Error\r\n', '', 1)
        if 'Connection closed due to timeout' in msg:
          raise ConnectionLostError('DGI reported timeout')
        else:
            raise RuntimeError('Received an error from DGI: ' + msg)
    elif msg.find('DeviceCommands\r\n') != 0:
        raise RuntimeError('Malformed command packet:\n' + msg)

    for line in msg.split('\r\n'):
        if line.find('DeviceCommands') == 0:
            continue
        command = line.split()
        if len(command) == 0:
            continue
        if len(command) != 3:
            raise RuntimeError('Malformed command in packet:\n' + line)        
        try:
            name = command[0]
            signal = command[1]
            value = float(command[2])
            get_device(name, devices).command_signal(signal, value)
        except NoSuchSignalError as e:
            raise RuntimeError('Packet contains invalid signal: '
                    + str(e))

    print 'Device states have been updated\n'


def work(adaptersock, devices, delay):
    """
    Send states, receive commands, then sleep for a bit.
    
    @param adaptersock the connected stream socket to use
    @param devices set of devices attached to this controller
    @param delay how long to wait before returning from this function

    @ErrorHandling caller must check for socket.timeout, socket.error, and
                   ConnectionLostError
    """
    send_states(adaptersock, devices)
    receive_commands(adaptersock, devices)
    time.sleep(delay)


def connect(devices, config):
    """
    Sends a Hello message to DGI, and receives its Start message in response.
    If there is a failure, tries again until it works.

    @param devices set of devices attached to this controller
    @param config various settings from the configuration file
    
    @return a stream socket connected to a DGI adapter
    """
    hello_packet = 'Hello\r\n'
    hello_packet += config['name'] + '\r\n'
    for device in devices:
        hello_packet += '{0} {1}\r\n'.format(device.get_type(),
                                             device.get_name())
    hello_packet += '\r\n'
    
    msg = ''
    while True:
        factorysock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        factorysock.settimeout(config['dgi-timeout'])
        try:
            print 'Attempting to connect to AdapterFactory...'
            factorysock.connect((config['host'], config['port']))
        except (socket.error, socket.herror, socket.gaierror, socket.timeout) \
                as e:
            print >> sys.stderr, \
                'Fail connecting to AdapterFactory: {0}'.format(str(e))
            time.sleep(config['hello-timeout'])
            print >> sys.stderr, 'Attempting to reconnect...'
            continue
        else:
            print 'Connected to AdapterFactory'

        try:
            send_all(factorysock, hello_packet)
            print '\nSent Hello message:\n' + hello_packet.strip()
            msg = recv_all(factorysock)
        except (socket.error, socket.timeout, ConnectionLostError) as e:
            print >> sys.stderr, \
                'AdapterFactory communication failure: {0}'.format(str(e))
            factorysock.close()
            time.sleep(config['hello-timeout'])
            print >> sys.stderr, 'Attempting to reconnect...'
        else:
            break

    if msg.find('Error') == 0:
        msg = msg.replace('Error', '', 1)
        print >> sys.stderr, 'Received an error from DGI: ' + msg
        time.sleep(config['hello-timeout'])
        return connect(devices, config)
    elif msg.find('BadRequest') == 0:
        handle_bad_request(msg)
    else:
        msg = msg.split()
        if len(msg) != 1 or msg[0] != 'Start':
            raise RuntimeError('DGI sent malformed Start:\n' + ' '.join(msg))
        else:
            print '\nReceived Start message:\n' + ' '.join(msg)

    # Note at this point, we're communicating with an adapter, not the factory
    return factorysock


def polite_quit(adaptersock, devices, rejected_delay):
    """
    Sends a quit request to DGI and returns once the request has been approved.
    If the request is not initially approved, continues to send device states
    and receive device commands until the request is approved or the DGI times
    out, at which point control returns to the script.

    @param adaptersock the connected stream socket which wants to quit
                       THIS SOCKET WILL BE CLOSED!!
    @param devices set of devices attached to this controller
    @param rejected_delay how long to wait after performing work, if rejected
    """
    while True:
        try:
            print 'Sending PoliteDisconnect request to DGI'
            send_all(adaptersock, 'PoliteDisconnect\r\n\r\n')
            msg = recv_all(adaptersock)
        except (socket.error, socket.timeout, ConnectionLostError) as e:
            print >> sys.stderr, \
                'DGI communication error: {0}'.format(str(e))
            print >> sys.stderr, 'Closing connection impolitely'
            adaptersock.close()
            return

        if msg.find('BadRequest') == 0:
            handle_bad_request(msg)
        elif msg.find('Error') == 0:
            msg = msg.replace('Error', '', 1)
            print >> sys.stderr, 'Received an error from DGI: ' + msg

        msg = msg.split()
        if len(msg) != 2 or msg[0] != 'PoliteDisconnect' or \
                (msg[1] != 'Accepted' and msg[1] != 'Rejected'):
            raise RuntimeError('Got bad disconnect response:\n' \
                    + ' '.join(msg))

        if msg[1] == 'Accepted':
            print 'PoliteDisconnect accepted by DGI'
            adaptersock.close()
            return
        else:
            assert msg[1] == 'Rejected'
            print 'PoliteDisconnect rejected, performing another round of work'
            try:
                work(adaptersock, devices, rejected_delay)
                # Loop again
            except (socket.error, socket.timeout, ConnectionLostError) as e:
                print >> sys.stderr, \
                    'DGI communication error: {0}'.format(str(e))
                print >> sys.stderr, 'Closing connection impolitely'
                adaptersock.close()
                return
