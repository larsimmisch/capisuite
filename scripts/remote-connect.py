import pcallcontrol as cc
import os, time
import commands

def acceptCall(CIP, callingParty, calledParty):
    print "nehme Anruf von",callingParty,"an",calledParty,"an."
    cc.connect(16) # 16 = telephony

def rejectCall(CIP, callingParty, calledParty):
    print "nehme Anruf von",callingParty,"an",calledParty,"nicht an."
    cc.reject(2) # 2 = normal call clearing


def establishInternetConnection():
    # establish connection
    timeout = time.time()+15 # 15 seconds timeout
    os.spawnl(os.P_NOWAIT, "ping", "ping", "-w", "1", "www.hillier.de")
    c_status = ""
    # wait 30 secs for connect
    while c_status != "CONNECTED" and time.time() < timeout:
        lines = commands.getoutput("/usr/sbin/cinternet --status")
        for l in lines.splitlines():
            if l.startswith("status"):
                c_status = l[9:].strip()
        print 'status:', c_status
        time.sleep(1)
    # get ip address
    save_lang = os.environ["LANG"]
    os.environ["LANG"] = ""
    lines = commands.getoutput("/sbin/ifconfig ppp0")
    os.environ["LANG"] = save_lang
    for l in lines: # no need to split into lines
        index = l.find("inet addr:")
        if index >= 0:
            index += 10
            rindex = l.find(" ", index)
            ip_address = l[index:rindex]
    # play ip address
    for i in ip_address:
        cc.audio_send(i+".la")


callWaitingMap = {
    '23': acceptCall,
    'default': denyCall,
    }

def callWaiting(CIP, callingParty, calledParty):
    action = callWaitingMap.get(calledParty, None)
    if action:
        action(CIP, callingParty, calledParty)
    else:
        action = callWaitingMap.get('default', None)
        if action:
            action(CIP, callingParty, calledParty)

callConnectedMap = {
    '3008': establishInternetConnection,
    }

def callConnected():
    try:
        cc.enableDTMF()
        cc.audio_send("send.la")
        code = cc.getDTMF()
        print "Bekommen habe ich",code
        action = callWaitingMap.get(calledParty, None)
        if action:
            action(CIP, callingParty, calledParty)
        else:
            action = callWaitingMap.get('default', None)
            if action:
                action(CIP, callingParty, calledParty)
        cc.disconnect()

    except cc.CallGoneError:
        print "schimpf schimpf: call gone"

cc.callWaiting = callWaiting
cc.callConnected = callConnected

