#                  idle.py - default script for capisuite
#              ---------------------------------------------
#    copyright            : (C) 2002 by Gernot Hillier
#    email                : gernot@hillier.de
#    version              : $Revision: 1.1 $
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

import os,re,time,pwd,fcntl
# capisuite stuff
import capisuite,cs_helpers

def idle(capi):
	config=cs_helpers.readConfig()
	done=config.get('GLOBAL','spool_dir')+"done/"
	failed=config.get('GLOBAL','spool_dir')+"failed/"
	if (not os.access(done,os.W_OK) or not os.access(failed,os.W_OK)):
		raise "Can't read/write to the necessary spool dirs"

	userlist=config.sections()
	userlist.remove('GLOBAL')

	for user in userlist: # search in all user-specified sendq's
		userdata=pwd.getpwnam(user)
		if (not config.has_option(user,"fax_numbers")):
			continue

		udir=config.get("GLOBAL","fax_user_dir")+user+"/"
		sendq=udir+"sendq/"
		if (not os.access(udir,os.F_OK)):
			os.mkdir(udir)
			os.chown(udir,userdata[2],userdata[3])
		if (not os.access(sendq,os.F_OK)):
			os.mkdir(sendq)
			os.chown(sendq,userdata[2],userdata[3])

		files=os.listdir(sendq)
		files=filter (lambda s: re.match("fax-.*\.txt",s),files)

		for job in files:
			job_fax=job[:-3]+"sff"
			real_user_c=os.stat(sendq+job).st_uid
			real_user_j=os.stat(sendq+job_fax).st_uid
			if (real_user_j!=pwd.getpwnam(user)[2] or real_user_c!=pwd.getpwnam(user)[2]):
				capisuite.error("job "+sendq+job_fax+" seems to be manipulated (wrong uid)! Ignoring...")
				fcntl.lockf(lockfile,fcntl.LOCK_UN)
				lockfile.close()
				os.unlink(sendq+job[:-3]+"lock")
				continue

			lockfile=open(sendq+job[:-3]+"lock","w")
			# read directory contents
			fcntl.lockf(lockfile,fcntl.LOCK_EX) # lock so that it isn't deleted while sending

			if (not os.access(sendq+job,os.W_OK)): # perhaps it was cancelled?
				fcntl.lockf(lockfile,fcntl.LOCK_UN)
				lockfile.close()
				os.unlink(sendq+job[:-3]+"lock")
				continue

			control=cs_helpers.readConfig(sendq+job)
			starttime=time.mktime(time.strptime(control.get("GLOBAL","starttime")))
			if (starttime>time.time()):
				fcntl.lockf(lockfile,fcntl.LOCK_UN)
				lockfile.close()
				os.unlink(sendq+job[:-3]+"lock")
				continue

			tries=control.getint("GLOBAL","tries")
			dialstring=control.get("GLOBAL","dialstring")
			mailaddress=cs_helpers.getOption(config,user,"fax_email")
			if (mailaddress=="" or mailaddress==None):
				mailaddress=user

			capisuite.log("job "+job_fax+" from "+user+" to "+dialstring+" initiated",1)
			result,resultB3 = sendfax(capi,sendq+job_fax,dialstring,user,config)

			capisuite.log("job "+job_fax+": result was %x,%x" % (result,resultB3),1)

			if (result in (0,0x3400,0x3480,0x3490) and resultB3==0):
				movejob(job_fax,sendq,done,user)
				capisuite.log("job "+job_fax+": finished successfully",1)
				cs_helpers.sendSimpleMail(user,mailaddress,"Fax to "+dialstring+" sent successfully.",
				"Your fax job to "+dialstring+" was sent successfully.\n\n"
				+"Filename: "+job_fax+"\nNeeded tries: "+str(tries)
				+("\nLast result: 0x%x/0x%x" % (result,resultB3))
				+"\n\nIt was moved to "+done+user+"-"+job_fax)
			else:
				max_tries=config.getint('GLOBAL','send_tries')
				delays=config.get('GLOBAL','send_delays').split(",")
				delays=map(int,delays)
				if (tries<len(delays)):
					next_delay=delays[tries]
				else:
					next_delay=delays[-1]
				starttime=time.time()+next_delay
				capisuite.log("job "+job_fax+": delayed for "+str(next_delay)+" seconds",2)
				tries+=1
				cs_helpers.writeDescription(sendq+job_fax,"dialstring=\""+dialstring+"\"\n"
				+"starttime=\""+time.ctime(starttime)+"\"\ntries=\""+str(tries)+"\"\n"
				+"user=\""+user+"\"")
				if (tries>=max_tries):
					movejob(job_fax,sendq,failed,user)
					capisuite.log("job "+job_fax+": failed finally",1)
					cs_helpers.sendSimpleMail(user,mailaddress,"Fax to "+dialstring+" FAILED.",
					"I'm sorry, but your fax job to "+dialstring+" failed finally.\n\n"
					+"Filename: "+job_fax+"\nTries: "+str(tries)
					+"\nLast result: 0x%x/0x%x" % (result,resultB3)
					+"\n\nIt was moved to "+failed+user+"-"+job_fax)

			fcntl.lockf(lockfile,fcntl.LOCK_UN)
			lockfile.close()
			os.unlink(sendq+job[:-3]+"lock")

def sendfax(capi,job,dialstring,user,config):
	try:
		outgoing_nr=cs_helpers.getOption(config,user,'outgoing_MSN')
		if (outgoing_nr==""):
			outgoing_nr=(config.get(user,'fax_numbers').split(','))[0]
		(call,result)=capisuite.call_faxG3(capi,int(config.get('GLOBAL','send_controller')),
		  outgoing_nr, dialstring, int(cs_helpers.getOption(config,user,'outgoing_timeout')),
		  cs_helpers.getOption(config,user,'fax_stationID'), cs_helpers.getOption(config,user,'fax_headline'))
		if (result!=0):
			return(result,0)
		capisuite.fax_send(call,job)
		return(capisuite.disconnect(call))
	except capisuite.CallGoneError:
		return(capisuite.disconnect(call))

def movejob(job,olddir,newdir,user):
	os.rename(olddir+job,newdir+user+"-"+job)
	os.rename(olddir+job[:-3]+"txt",newdir+user+"-"+job[:-3]+"txt")

#
# History:
#
# $Log: idle.py,v $
# Revision 1.1  2003/02/19 08:19:54  gernot
# Initial revision
#
# Revision 1.12  2003/02/18 09:54:22  ghillie
# - added missing lockfile deletions, corrected locking protocol
#   -> fixes Bugzilla 23731
#
# Revision 1.11  2003/02/17 16:48:43  ghillie
# - do locking, so that jobs can be deleted
#
# Revision 1.10  2003/02/10 14:50:52  ghillie
# - revert logic of outgoing_MSN: it's overriding the first number of
#   fax_numbers now
#
# Revision 1.9  2003/02/05 15:59:11  ghillie
# - search for *.txt instead of *.sff so no *.sff which is currently created
#   by capisuitefax will be found!
#
# Revision 1.8  2003/01/31 11:22:00  ghillie
# - use different sendq's for each user (in his user_dir).
# - use prefix user- for names in done and failed
#
# Revision 1.7  2003/01/27 21:56:46  ghillie
# - mailaddress may be not set, that's the same as ""
# - use first entry of fax_numbers as outgoing MSN if it exists
#
# Revision 1.6  2003/01/27 19:24:29  ghillie
# - updated to use new configuration files for fax & answering machine
#
# Revision 1.5  2003/01/19 12:03:27  ghillie
# - use capisuite log functions instead of stdout/stderr
#
# Revision 1.4  2003/01/17 15:09:26  ghillie
# - updated to use new configuration file capisuite-script.conf
#
# Revision 1.3  2003/01/13 16:12:00  ghillie
# - renamed from idle.pyin to idle.py as all previously processed variables
#   stay in the config file and cs_helpers.pyin now
#
# Revision 1.2  2002/12/16 13:07:22  ghillie
# - finished queue processing
#
# Revision 1.1  2002/12/14 13:53:19  ghillie
# - idle.py and incoming.py are now auto-created from *.pyin
#

