#This makefile is for MSYS/MinGW systems

CC=g++
CFLAGS=-c `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gmodule-export-2.0` -Wl,--export-dynamic
#LIBS=`pkg-config --libs gtk+-2.0` -L. -lmobot -linterface
LIBS=`pkg-config --libs gtk+-2.0` `pkg-config --libs gmodule-export-2.0` -L. -linterface

#OBJS=RoboMancer.o  connectHandlers.o  gait.o  menuHandlers.o  movementFunctions.o  movementHandlers.o
OBJS=connectDialog.o RoboMancer.o 
HEADERS=RoboMancer.h

#all:libmobot.a $(OBJS) RoboMancer
all:$(OBJS) RoboMancer

MOBOTDIR=/d/Projects/iMobot_gumstix_api/libimobotcomms

#mobot.dll:$(MOBOTDIR)/Release/mobot.dll
#	cp $(MOBOTDIR)/Release/mobot.dll mobot.dll

#mobot.lib:$(MOBOTDIR)/Release/mobot.lib
#	cp $(MOBOTDIR)/Release/mobot.lib mobot.lib

#mobot.h:$(MOBOTDIR)/mobot.h
#	cp $(MOBOTDIR)/mobot.h mobot.h

#mobot.def:mobot.dll mobot.lib
#	pexports mobot.dll | sed 's/^_//' > mobot.def

#libmobot.a:mobot.def mobot.dll
#	dlltool --input-def mobot.def --dllname mobot.dll --output-lib libmobot.a -k
#	ranlib libmobot.a

.cpp.o:
	$(CC) $(CFLAGS) $<

interface.o:interface/interface.glade
	ld -r -b binary -o interface.o interface/interface.glade

libinterface.a:interface.o
	ar rcs libinterface.a interface.o

RoboMancer:$(OBJS) $(HEADERS) libinterface.a
	$(CC) -v $(OBJS) $(LIBS) -o RoboMancer

clean:
	rm -rf RoboMancer $(OBJS) libinterface.a interface.o 
	#rm -rf RoboMancer $(OBJS) libinterface.a interface.o libmobot.a mobot.def mobot.lib mobot.dll
