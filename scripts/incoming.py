#              incoming.py - standard incoming script for capisuite
#              ----------------------------------------------------
#    copyright            : (C) 2002 by Gernot Hillier
#    email                : gernot@hillier.de
#    version              : $Revision: 1.1 $
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

# general imports
import time,os,re,string,pwd
# CapiSuite imports
import capisuite,cs_helpers

# @brief main function called by CapiSuite when an incoming call is received
#
# It will decide if this call should be accepted, with which service and for
# which user. The real call handling is done in faxIncoming and voiceIncoming.
#
# @param call reference to the call. Needed by all capisuite functions
# @param service one of SERVICE_FAXG3, SERVICE_VOICE, SERVICE_OTHER
# @param call_from string containing the number of the calling party
# @param call_to string containing the number of the called party
def callIncoming(call,service,call_from,call_to):
	# read config file and search for call_to in the user sections
	try:
		config=cs_helpers.readConfig()
		userlist=config.sections()
		userlist.remove('GLOBAL')
		curr_user=""

		for u in userlist:
			if config.has_option(u,'voice_numbers'):
				numbers=config.get(u,'voice_numbers')
				if (call_to in numbers.split(',') or numbers=="*"):
					if (service==capisuite.SERVICE_VOICE):
						curr_user=u
						curr_service=capisuite.SERVICE_VOICE
						break
					if (service==capisuite.SERVICE_FAXG3):
						curr_user=u
						curr_service=capisuite.SERVICE_FAXG3
						break

			if config.has_option(u,'fax_numbers'):
				numbers=config.get(u,'fax_numbers')
				if (call_to in numbers.split(',') or numbers=="*"):
				 	if (service in (capisuite.SERVICE_FAXG3,capisuite.SERVICE_VOICE)):
						curr_user=u
						curr_service=capisuite.SERVICE_FAXG3
						break

	except IOError,e:
		capisuite.error("Error occured during config file reading: "+e+" Disconnecting...")
		capisuite.reject(call,0x34A9)
		return
        # setuid to the user and answer the call with the right service
	if (curr_user==""):
		capisuite.log("call from "+call_from+" to "+call_to+" ignoring",1,call)
		capisuite.reject(call,1)
		return
	try:
		try:
			userdata=pwd.getpwnam(curr_user)
			if (curr_service==capisuite.SERVICE_VOICE):
				udir=config.get("GLOBAL","voice_user_dir")+curr_user+"/"
			elif (curr_service==capisuite.SERVICE_FAXG3):
				udir=config.get("GLOBAL","fax_user_dir")+curr_user+"/"
			if (not os.access(udir,os.F_OK)):
				os.mkdir(udir)
				os.chown(udir,userdata[2],userdata[3])
			if (not os.access(udir+"received/",os.F_OK)):
				os.mkdir(udir+"received/")
				os.chown(udir+"received/",userdata[2],userdata[3])
			os.setuid(userdata[2])
		except KeyError:
			capisuite.error("user "+curr_user+" is not a valid system user. Disconnecting",call)
			capisuite.reject(call,0x34A9)
			return
		if (curr_service==capisuite.SERVICE_VOICE):
			capisuite.log("call from "+call_from+" to "+call_to+" for "+curr_user+" connecting with voice",1,call)
			capisuite.connect_voice(call,int(cs_helpers.getOption(config,curr_user,"voice_delay")))
			voiceIncoming(call,call_from,call_to,curr_user,config)
		elif (curr_service==capisuite.SERVICE_FAXG3):
			capisuite.log("call from "+call_from+" to "+call_to+" for "+curr_user+" connecting with fax",1,call)
			capisuite.connect_faxG3(call,cs_helpers.getOption(config,curr_user,"fax_stationID"),cs_helpers.getOption(config,curr_user,"fax_headline"),0)
			faxIncoming(call,call_from,call_to,curr_user,config)
	except capisuite.CallGoneError: # catch exceptions from connect_*
		(cause,causeB3)=capisuite.disconnect(call)
		capisuite.log("connection lost with cause 0x%x,0x%x" % (cause,causeB3),1,call)

# @brief called by callIncoming when an incoming fax call is received
#
# @param call reference to the call. Needed by all capisuite functions
# @param call_from string containing the number of the calling party
# @param call_to string containing the number of the called party
# @param curr_user name of the user who is responsible for this
# @param config ConfigParser instance holding the config data
def faxIncoming(call,call_from,call_to,curr_user,config):
	filename=cs_helpers.uniqueName(config.get("GLOBAL","fax_user_dir")+curr_user+"/received/","fax","sff")
	try:
		capisuite.fax_receive(call,filename)
		(cause,causeB3)=capisuite.disconnect(call)
		capisuite.log("connection finished with cause 0x%x,0x%x" % (cause,causeB3),1,call)

	except capisuite.CallGoneError: # catch this here to get the cause info in the mail
		(cause,causeB3)=capisuite.disconnect(call)
		capisuite.log("connection lost with cause 0x%x,0x%x" % (cause,causeB3),1,call)

	if (os.access(filename,os.R_OK)):
		cs_helpers.writeDescription(filename,
		  "call_from=\""+call_from+"\"\ncall_to=\""+call_to+"\"\ntime=\""
		  +time.ctime()+"\"\ncause=\"0x%x/0x%x\"\n" % (cause,causeB3))

		mailaddress=cs_helpers.getOption(config,curr_user,"fax_email")
		if (mailaddress=="" or mailaddress==None):
			mailaddress=curr_user
		if (cs_helpers.getOption(config,curr_user,"fax_action").lower()=="mailandsave"):
			cs_helpers.sendMIMEMail(curr_user, mailaddress, "Fax received from "+call_from+" to "+call_to, "sff",
			  "You got a fax from "+call_from+" to "+call_to+"\nDate: "+time.ctime()+"\n\n"
			  +"See attached file.\nThe original file was saved to "+filename+"\n\n", filename)

# @brief called by callIncoming when an incoming voice call is received
#
# @param call reference to the call. Needed by all capisuite functions
# @param call_from string containing the number of the calling party
# @param call_to string containing the number of the called party
# @param curr_user name of the user who is responsible for this
# @param config ConfigParser instance holding the config data
def voiceIncoming(call,call_from,call_to,curr_user,config):
	userdir=config.get("GLOBAL","voice_user_dir")+curr_user+"/"
	filename=cs_helpers.uniqueName(userdir+"received/","voice","la")
	try:
		capisuite.enable_DTMF(call)
		userannouncement=userdir+cs_helpers.getOption(config,curr_user,"announcement")
		pin=cs_helpers.getOption(config,curr_user,"pin")
		if (os.access(userannouncement,os.R_OK)):
			capisuite.audio_send(call,userannouncement,1)
		else:
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"anrufbeantworter-von.la"),1)
			cs_helpers.sayNumber(call,call_to,curr_user,config)
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"bitte-nachricht.la"),1)

		if (cs_helpers.getOption(config,curr_user,"voice_action").lower()!="none"):
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"beep.la"),1)
			capisuite.audio_receive(call,filename,int(cs_helpers.getOption(config,curr_user,"record_length")), int(cs_helpers.getOption(config,curr_user,"record_silence_timeout")),1)

		dtmf_list=capisuite.read_DTMF(call,0)
		if (dtmf_list=="X"):
			if (os.access(filename,os.R_OK)):
				os.unlink(filename)
			capisuite.switch_to_faxG3(call,cs_helpers.getOption(config,curr_user,"fax_stationID"),cs_helpers.getOption(config,curr_user,"fax_headline"))
			faxIncoming(call,call_from,call_to,curr_user,config)
		elif (dtmf_list!="" and pin!=""):
			dtmf_list+=capisuite.read_DTMF(call,3) # wait 5 seconds for input
			count=1
			while (count<3 and pin!=dtmf_list):  # try again if input was wrong
				capisuite.log("wrong PIN entered...",1,call)
				capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"beep.la"))
				dtmf_list=capisuite.read_DTMF(call,3)
				count+=1
			if (pin==dtmf_list):
				if (os.access(filename,os.R_OK)):
					os.unlink(filename)
				capisuite.log("Starting remote inquiry...",1,call)
				remoteInquiry(call,userdir,curr_user,config)

		(cause,causeB3)=capisuite.disconnect(call)
		capisuite.log("connection finished with cause 0x%x,0x%x" % (cause,causeB3),1,call)

	except capisuite.CallGoneError: # catch this here to get the cause info in the mail
		(cause,causeB3)=capisuite.disconnect(call)
		capisuite.log("connection lost with cause 0x%x,0x%x" % (cause,causeB3),1,call)

	if (os.access(filename,os.R_OK)):
		cs_helpers.writeDescription(filename,
		  "call_from=\""+call_from+"\"\ncall_to=\""+call_to+"\"\ntime=\""
		  +time.ctime()+"\"\ncause=\"0x%x/0x%x\"\n" % (cause,causeB3))

		mailaddress=cs_helpers.getOption(config,curr_user,"voice_email")
		if (mailaddress=="" or mailaddress==None):
			mailaddress=curr_user
		if (cs_helpers.getOption(config,curr_user,"voice_action").lower()=="mailandsave"):
			cs_helpers.sendMIMEMail(curr_user, mailaddress, "Voice call received from "+call_from+" to "+call_to, "la",
			  "You got a voice call from "+call_from+" to "+call_to+"\nDate: "+time.ctime()+"\n\n"
			  +"See attached file.\nThe original file was saved to "+filename+"\n\n", filename)


# @brief remote inquiry function (uses german wave snippets!)
#
# commands for remote inquiry
# delete message - 1
# next message - 4
# last message - 5
# repeat current message - 6
#
# @param call reference to the call. Needed by all capisuite functions
# @param userdir spool_dir of the current_user
# @param curr_user name of the user who is responsible for this
# @param config ConfigParser instance holding the config data
def remoteInquiry(call,userdir,curr_user,config):
	import time,fcntl,errno,os
	# acquire lock
	lockfile=open(userdir+"received/inquiry_lock","w")
	try:
		try:
			# read directory contents
			fcntl.lockf(lockfile,fcntl.LOCK_EX | fcntl.LOCK_NB) # only one inquiry at a time!

			messages=os.listdir(userdir+"received/")
			messages=filter (lambda s: re.match("voice-.*\.la",s),messages)  # only use voice-* files
			messages=map(lambda s: int(re.match("voice-([0-9]+)\.la",s).group(1)),messages) # filter out numbers
			messages.sort()
			
			# read the number of the message heard last at the last inquiry
			lastinquiry=-1
                        if (os.access(userdir+"received/last_inquiry",os.W_OK)):
				lastfile=open(userdir+"received/last_inquiry","r")
				lastinquiry=int(lastfile.readline())
				lastfile.close()
			print lastinquiry

			# sort out old messages
			oldmessages=[]
			i=0
			while (i<len(messages)):
				oldmessages.append(messages[i])
				if (messages[i]<=lastinquiry):
					del messages[i]
				else:
					i+=1
			print "messages",messages
			print "oldmessages",oldmessages

			cs_helpers.sayNumber(call,str(len(messages)),curr_user,config)
			if (len(messages)==1):
				capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"neue-nachricht.la"),1)
			else:
				capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"neue-nachrichten.la"),1)

			# menu for record new announcement
			cmd=""
			while (cmd not in ("1","9")):
				if (len(oldmessages)):
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"zum-abhoeren-1.la"),1)
				capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"fuer-neue-ansage-9.la"),1)
				cmd=capisuite.read_DTMF(call,0,1)
			if (cmd=="9"):
				newAnnouncement(call,userdir,curr_user,config)
				return

			# start inquiry
			for curr_msgs in (messages,oldmessages):
				cs_helpers.sayNumber(call,str(len(curr_msgs)),curr_user,config)
				if (curr_msgs==messages):
					if (len(curr_msgs)==1):
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"neue-nachricht.la"),1)
					else:
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"neue-nachrichten.la"),1)
				else:
					if (len(curr_msgs)==1):
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"nachricht.la"),1)
					else:
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"nachrichten.la"),1)

				i=0
				while (i<len(curr_msgs)):                                    
					filename=userdir+"received/voice-"+str(curr_msgs[i])+".la"
					descr=cs_helpers.readConfig(filename[:-2]+"txt")
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"nachricht.la"),1)
					cs_helpers.sayNumber(call,str(i+1),curr_user,config)
					if (descr.get('GLOBAL','call_from')!="??"):
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"von.la"),1)
						cs_helpers.sayNumber(call,descr.get('GLOBAL','call_from'),curr_user,config)
					if (descr.get('GLOBAL','call_to')!="??"):
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"fuer.la"),1)
						cs_helpers.sayNumber(call,descr.get('GLOBAL','call_to'),curr_user,config)
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"am.la"),1)
					calltime=time.strptime(descr.get('GLOBAL','time'))
					cs_helpers.sayNumber(call,str(calltime[2]),curr_user,config)
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"..la"),1)
					cs_helpers.sayNumber(call,str(calltime[1]),curr_user,config)
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"..la"),1)
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"um.la"),1)
					cs_helpers.sayNumber(call,str(calltime[3]),curr_user,config)
					capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"uhr.la"),1)
					cs_helpers.sayNumber(call,str(calltime[4]),curr_user,config)
					capisuite.audio_send(call,filename,1)
					cmd=""
					while (cmd not in ("1","4","5","6")):
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"erklaerung.la"),1)
						cmd=capisuite.read_DTMF(call,0,1)
					if (cmd=="1"):
						os.remove(filename)
						os.remove(filename[:-2]+"txt")
						if (curr_msgs==messages): # if we are in new message mode...
							oldmessages.remove(curr_msgs[i]) # ... don't forget to delete it in both lists
						del curr_msgs[i]
						capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"nachricht-gelöscht.la"))
					elif (cmd=="4"):
						if (curr_msgs[i]>lastinquiry):
							lastinquiry=curr_msgs[i]
							lastfile=open(userdir+"received/last_inquiry","w")
							lastfile.write(str(curr_msgs[i])+"\n")
							lastfile.close()
						i+=1
					elif (cmd=="5"):
						i-=1
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"keine-weiteren-nachrichten.la"))

		except IOError,err:
			if (err.errno in (errno.EACCES,errno.EAGAIN)):
				capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"fernabfrage-aktiv.la"))
	finally:
		# unlock
		fcntl.lockf(lockfile,fcntl.LOCK_UN)
		lockfile.close()
		os.unlink(userdir+"received/inquiry_lock")

# @brief remote inquiry: record new announcement (uses german wave snippets!)
#
# @param call reference to the call. Needed by all capisuite functions
# @param userdir spool_dir of the current_user
# @param curr_user name of the user who is responsible for this
# @param config ConfigParser instance holding the config data
def newAnnouncement(call,userdir,curr_user,config):
	capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"bitte-neue-ansage-komplett.la"))
	capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"beep.la"))
	cmd=""
	while (cmd!="1"):
		capisuite.audio_receive(call,userdir+"announcement-tmp.la",60,3)
		capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"neue-ansage-lautet.la"))
		capisuite.audio_send(call,userdir+"announcement-tmp.la")
		capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"wenn-einverstanden-1.la"))
		cmd=capisuite.read_DTMF(call,0,1)
		if (cmd!="1"):
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"bitte-neue-ansage-kurz.la"))
			capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"beep.la"))
	userannouncement=userdir+cs_helpers.getOption(config,curr_user,"announcement")
	os.rename(userdir+"announcement-tmp.la",userannouncement)
	capisuite.audio_send(call,cs_helpers.getAudio(config,curr_user,"ansage-gespeichert.la"))

#
# History:
#
# $Log: incoming.py,v $
# Revision 1.1  2003/02/19 08:19:54  gernot
# Initial revision
#
# Revision 1.11  2003/02/17 11:13:43  ghillie
# - remoteinquiry supports new and old messages now
#
# Revision 1.10  2003/02/03 14:50:08  ghillie
# - fixed small typo
#
# Revision 1.9  2003/01/31 16:32:41  ghillie
# - support "*" for "all numbers"
# - automatic switch voice->fax when SI says fax
#
# Revision 1.8  2003/01/31 11:24:41  ghillie
# - wrong user handling for more than one users fixed
# - creates user_dir/user and user_dir/user/received now separately as
#   idle.py can also create user_dir/user now
#
# Revision 1.7  2003/01/27 21:57:54  ghillie
# - fax_numbers and voice_numbers may not exist (no fatal error any more)
# - accept missing email option
# - fixed typo
#
# Revision 1.6  2003/01/27 19:24:29  ghillie
# - updated to use new configuration files for fax & answering machine
#
# Revision 1.5  2003/01/19 12:03:27  ghillie
# - use capisuite log functions instead of stdout/stderr
#
# Revision 1.4  2003/01/17 15:09:49  ghillie
# - cs_helpers.sendMail was renamed to sendMIMEMail
#
# Revision 1.3  2003/01/16 12:58:34  ghillie
# - changed DTMF timeout for pin to 3 seconds
# - delete recorded wave if fax or remote inquiry is recognized
# - updates in remoteInquiry: added menu for recording own announcement
# - fixed some typos
# - remoteInquiry: delete description file together with call if requested
# - new function: newAnnouncement
#
# Revision 1.2  2003/01/15 15:55:12  ghillie
# - added exception handler in callIncoming
# - faxIncoming: small typo corrected
# - voiceIncoming & remoteInquiry: updated to new config file system
#
# Revision 1.1  2003/01/13 16:12:58  ghillie
# - renamed from incoming.pyin to incoming.py as all previously processed
#   variables are moved to config and cs_helpers.pyin
#
# Revision 1.4  2002/12/18 14:34:56  ghillie
# - added some informational prints
# - accept voice calls to fax nr
#
# Revision 1.3  2002/12/16 15:04:51  ghillie
# - added missing path prefix to delete routing in remote inquiry
#
# Revision 1.2  2002/12/16 13:09:25  ghillie
# - added some comments about the conf_* vars
# - added conf_wavedir
# - added support for B3 cause now returned by disconnect()
# - corrected some dir entries to work in installed system
#
# Revision 1.1  2002/12/14 13:53:18  ghillie
# - idle.py and incoming.py are now auto-created from *.pyin
#
# Revision 1.4  2002/12/11 12:58:05  ghillie
# - read return value from disconnect()
# - added disconnect() to exception handler
#
# Revision 1.3  2002/12/09 15:18:35  ghillie
# - added disconnect() in exception handler
#
# Revision 1.2  2002/12/02 21:30:42  ghillie
# fixed some minor typos
#
# Revision 1.1  2002/12/02 21:15:55  ghillie
# - moved scripts to own directory
# - added remote-connect script to repository
#
# Revision 1.20  2002/12/02 20:59:44  ghillie
# another typo :-|
#
# Revision 1.19  2002/12/02 20:54:07  ghillie
# fixed small typo
#
# Revision 1.18  2002/12/02 16:51:32  ghillie
# nearly complete new script, supports answering machine, fax receiving and remote inquiry now
#
# Revision 1.17  2002/11/29 16:28:43  ghillie
# - updated syntax (connect_telephony -> connect_voice)
#
# Revision 1.16  2002/11/29 11:09:04  ghillie
# renamed CapiCom to CapiSuite (name conflict with MS crypto API :-( )
#
# Revision 1.15  2002/11/25 11:43:43  ghillie
# updated to new syntax
#
# Revision 1.14  2002/11/23 16:16:17  ghillie
# moved switch2fax after audio_receive()
#
# Revision 1.13  2002/11/22 15:48:58  ghillie
# renamed pcallcontrol module to capicom
#
# Revision 1.12  2002/11/22 15:02:39  ghillie
# - added automatic switch between speech and fax
# - some comments added
#
# Revision 1.11  2002/11/19 15:57:18  ghillie
# - Added missing throw() declarations
# - phew. Added error handling. All exceptions are caught now.
#
# Revision 1.10  2002/11/18 12:32:36  ghillie
# - callIncoming lives now in __main__, not necessarily in pcallcontrol any more
# - added some comments and header
#
