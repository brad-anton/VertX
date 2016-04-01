#!/usr/bin/env python
#
# VertX_Query.py - HID VertX Discovery and Query Tool
# by brad antoniewicz
#


import struct
import socket
import binascii
import getopt
import sys
import random
from socket import *

def usage():
        help = "Options:\n"
        help += "\t-h <host>\t host\n"
        help += "\t-p <port>\t port (default 4070)\n"
        help += "\t-v \t\t verbose\n"
        help += "\t-m <code> \t Message Type code\n"
        help += "\nSupported Message Types:\n"
        help += "\t01 \t Discover\n"
        help += "\t02 \t command_blink_on\n"
        help += "\t03 \t command_blink_off\n"
        return help

def process_resp(recv_data,msg_type):
        # ('discovered;090;MAC_ADDR;VertXController;IP_ADDR;2;V2000;2.2.7.18;02/27/2007;', ('10.112.18.50', 4070))
        mac=0
        if msg_type == "01":
                data = recv_data[0].split(';')
                print "\tType:\t\t",data[3],"-",data[6]
                print "\tVersion:\t",data[7]
                print "\tIP Address:\t",data[4]
                print "\tMAC Address:\t",data[2]
                mac = data[2]
        return mac

def get_mac(host,port):
        msg_to_send = buildmsg("01",0,host,port)
        if msg_to_send:
                mac = sendmsg(host,port,binascii.unhexlify(msg_to_send),0,0,"01")
        return mac

def buildmsg(msg_type,verbose,host,port):
        # Basic Connect
        if msg_type == "01" :
                # discover;013;
                msg = "646973636f7665723b3031333b"
        elif msg_type == "02":
                # command_blink_on;042;MAC_ADDR;30;
                print "[+] Querying Host first to pull MAC address"
                msg = "command_blink_on;042;"
                msg += get_mac(host,port)
                msg += ";30;"
                msg = binascii.hexlify(msg)
                print "[+] Sending command_blink_on"

        elif msg_type == "03":
                # command_blink_off;042;MAC_ADDR;1;
                print "[+] Querying host first to pull MAC address"
                msg = "command_blink_off;042;"
                msg += get_mac(host,port)
                msg += ";1;"
                msg = binascii.hexlify(msg)
                print "[+] Sending command_blink_off"
        else :
                print "[!] Message Type", msg_type, "unsupported!"
                print "[!] Unsupported message types can only be used with -f\n"
                print "[!] Quiting..."
                return 0

        ## Get the length of our msg
        total_length = len(binascii.unhexlify(msg))

        datahex=msg

        if verbose:
                print '[Verbose] Message before Send:\nH(', total_length, '):', datahex, '\t | \tA:', binascii.unhexlify(datahex)

        return datahex


def sendmsg(host,port,hex_msg,verbose,count,msg_type):

        udp=0
        
        #sss0bbb 2016-04-01 added to correct UnboundLocalError 
        mac=0
        
        if ((msg_type == "01") or (msg_type == "02") or (msg_type == "03")):
                if verbose:
                        print '[Verbose] Message type indicates UDP'
                udp=1

        if udp:
                s = socket(AF_INET,SOCK_DGRAM)
                if ( host == "255.255.255.255"):
                        if verbose:
                                print '[Verbose] Destination is Broadcast'
                        s.bind(('',0))
                        s.setsockopt(SOL_SOCKET, SO_BROADCAST,1)
        else:
                s = socket.socket()

        s.settimeout(2)

        recv_data = 0

        if verbose:
                print '[+] Target',host,':',port
                print "[+] Sending msg with length",len(hex_msg)

        if udp:
                s.sendto(hex_msg,(host,port))
                error=0
        else:
                s.connect((host,port))
                error = s.sendall(hex_msg)

        if error:
                print "[!] Error:",error
        else:
                try:
                        if udp:
                                 recv_data = s.recvfrom(1024);
                        else:
                                recv_data = s.recv(1024);

                except:
                        if count:
                                print "[",count,":ALERT] Client timed out or didnt respond!"
                        else:
                                print "[ALERT] Client timed out or didnt respond!"

                if recv_data:
                        if verbose:
                                print '[+] Client Resp:'
                                print '\t ascii:\t',recv_data
                                if not udp:
                                        print '\t hex:\t',binascii.hexlify(recv_data)
                        else:
                                print "[+] Got Response"
                        mac = process_resp(recv_data,msg_type)
                else:
                        if count:
                                print "[",count,"] No recv_data"
                        else:
                                print "[+] No recv_data"

        if not udp:
                s.shutdown(2)

        s.close()
        if mac:
                return mac


def main():
        print "VertX_Query.py - HID VertX Discovery and Query Tool"
        print "by brad antoniewicz"
        print "--------------------------------------\n"

        try:
                opts, args = getopt.getopt(sys.argv[1:], "h:p:m:vfs:at:",[])

        except getopt.GetoptError:
                print usage()
                return
        port = 4070
        host = cmdcode = verbose = fuzz = hex_str = dumbfuzz = ftype = vals = 0

        for o, a in opts:
                if o == "-h":
                        host = a
                if o == "-p":
                        port = int(a)
                if o == "-m":
                        cmdcode = a
                if o == "-v":
                        verbose = 1
                if o == "-f":
                        fuzz = 1
                if o == "-s":
                        hex_str = a
                if o == "-a":
                        vals = 1
                if o == "-t":
                        ftype = a
        if (host == 0) or (cmdcode == 0):
                print usage()
                return

        if verbose and cmdcode:
                print "[Verbose] Command code: ",cmdcode

        if fuzz:
                fuzz_msg(host,port,cmdcode,hex_str,verbose,ftype,vals)
        elif dumbfuzz:
                dumbfuzzer(verbose)
        else:
                msg_to_send = buildmsg(cmdcode,verbose,host,port)
                if msg_to_send:
                        sendmsg(host,port,binascii.unhexlify(msg_to_send),verbose,0,cmdcode)
main()
