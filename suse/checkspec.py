#!/usr/bin/python
#
# Test a spec file against INDEX.gz to see if all "usedforbuild" packages are
# valid, remove packages which are automatically added by build
#

# default files, taken from /usr/lib/build/init_buildsystem
basefiles="aaa_base","bash","db","devs","diffutils","gdbm","glibc","grep",\
           "m4","ncurses","pam","perl","readline","rpm"
secondfiles="cpp","file","findutils","gawk","mktemp","shadow","timezone",\
	    "util-linux"

files={}	   
files["7.3"]=files["8.0"]=basefiles+("base","textutils","sh-utils",\
			  "fileutils","aaa_dir")+secondfiles
files["8.1"]=basefiles+("fillup","sed","acl","attr","textutils","sh-utils",\
	     "fileutils","filesystem","libxcrypt","zlib","pam-modules", \
	     "permissions")+secondfiles+("bind9-utils",)
files["8.2"]=basefiles+("fillup","insserv","sed","libacl","libattr","attr",\
	     "acl","filesystem","libxcrypt","zlib","coreutils","pam-modules",\
	     "permissions")+secondfiles+("bind9-utils",)

import sys,gzip,os,string

# --- read commandline params ---

if (len(sys.argv)<3): 
	print "usage: checkspec.py <specname> <suse-version> [<indexgz>]"
	sys.exit(0)

specfile=open(sys.argv[1])
suseversion=sys.argv[2]

if (suseversion not in files.keys()):
	print "unknown SuSE version!"
	sys.exit(0)

if (len(sys.argv)>=4):
	index=gzip.GzipFile(sys.argv[3])
else: 
	index=gzip.GzipFile("/media/dvd/INDEX.gz")

# --- read SPEC file ---

line=" "
while (line[0:14]!="# usedforbuild" and line!=""):
	line=specfile.readline()

if (line==""):
	print "usedforbuild not found!!"
	sys.exit(1)

usedforbuild=line.split()
del usedforbuild[0:2]

# --- read INDEX.gz ---

line=" "
distrifiles=[]
while (line!=""):
	line=index.readline().rstrip()
	entry=os.path.basename(line).split(".")
	if (line.find("suse")!=-1  and entry[-1]=="rpm"):
		if (entry[-2] in ("i386","i486","i586","i686","nosrc","noarch")):
			# join split up version number, strip off .xxx.rpm
			entry=string.join(entry[0:-2],".")
			# split of last two version numbers (pkg + rpm version)
			entry=string.join(entry.split("-")[0:-2],"-")
			distrifiles.append(entry)	

# --- filter out 

for pkg in usedforbuild:
	if (pkg in files[suseversion]):
		usedforbuild.remove(pkg)
	else:
		if (pkg not in distrifiles):
			print "problematic package: ",pkg
			usedforbuild.remove(pkg)

print "\n\nsuggested new usedforbuild line:\n"
print "# usedforbuild   "+string.join(usedforbuild," ")

