import string,os,time
import pcallcontrol
cc=pcallcontrol

def callWaiting(CIP,callingParty,calledParty):
	if (calledParty=="23"):
		print "nehme Anruf von",callingParty,"an",calledParty,"an."
		cc.connect(16) # 16 = telephony
	else:
		print "nehme Anruf von",callingParty,"an",calledParty,"nicht an."
		cc.reject(2) # 2 = normal call clearing

def callConnected():
	try:
		cc.enableDTMF()
		cc.audio_send("send.la")
		code=cc.getDTMF()
		print "Bekommen habe ich",code
		if code=="3008":
			# establish connection
			start_t=time.time()
			os.spawnl(os.P_NOWAIT,"ping","ping","-w","1","www.hillier.de")
			c_status=""
			# wait 30 secs for connect
			while ((c_status!="CONNECTED") and (time.time()<start_t+15)):
				cmd=os.popen("/usr/sbin/cinternet --status")
				lines=cmd.readlines()
				cmd.close()
				for l in lines:
					if (l[0:6]=="status"):
						c_status=l[9:]
				c_status=string.strip(c_status)
				print c_status
				time.sleep(1)
			# get ip address
			save_lang=os.environ["LANG"]
			os.environ["LANG"]=""
			cmd=os.popen("/sbin/ifconfig ppp0")
			lines=cmd.readlines()
			cmd.close()
			os.environ["LANG"]=save_lang
			for l in lines:
        			index=string.find(l,"inet addr:")
        			if (index!=-1):
                			index+=10
                			rindex=string.find(l," ",index)
                			ip_address=l[index:rindex]
			# play ip address
			for i in ip_address:
                		cc.audio_send(i+".la")

		cc.disconnect()

	except cc.CallGoneError:
		print "schimpf schimpf"

cc.callWaiting=callWaiting
cc.callConnected=callConnected

