#OUTNAME=autodll
#ROOTDIR=C:\tmp\CustomModels
#LCCROOT=c:\lcc

OUTDIR=$(ROOTDIR)\dll
SRCDIR=$(ROOTDIR)\$(OUTNAME)
HEADERS=$(ROOTDIR)\headers
LCCHEADERS=$(LCCROOT)\include
CFLAGS=-I"$(LCCHEADERS)" -O
CC=$(LCCROOT)\bin\lcc.exe
LINKER=$(LCCROOT)\bin\lcclnk.exe


OBJS=\
	$(OUTNAME).obj\
	devicedllutilities.obj

LIBS=

EXE=$(OUTNAME).dll

$(EXE):	$(OBJS) makefile
	"$(LINKER)"  -s -dll -nounderscores -o "$(OUTDIR)\$(OUTNAME).dll" $(OBJS) $(LIBS)
	if exist "$(SRCDIR)\devicedllutilities.obj" del "$(SRCDIR)\devicedllutilities.obj"
	if exist "$(SRCDIR)\$(OUTNAME).obj" del "$(SRCDIR)\$(OUTNAME).obj"
	if exist "$(OUTDIR)\$(OUTNAME).lib" del "$(OUTDIR)\$(OUTNAME).lib"
	if exist "$(OUTDIR)\$(OUTNAME).exp" del "$(OUTDIR)\$(OUTNAME).exp"
	if exist "$(HEADERS)\$(OUTNAME).obj" del "$(HEADERS)\$(OUTNAME).obj"
	if exist "$(HEADERS)\devicedllutilities.obj" del "$(HEADERS)\devicedllutilities.obj"


DEVICEDLLUTILITIES_C=\
	"$(HEADERS)\DeviceDLLUtilities.h"\
	"$(HEADERS)\DLLStructs.h"\
	"$(HEADERS)\DeviceTypes.h"\
	"$(HEADERS)\DeviceDLL.h"


$(OUTNAME).obj: $(DEVICEDLLUTILITIES_C) "$(SRCDIR)\$(OUTNAME).c" 
	"$(CC)" -c $(CFLAGS) "$(SRCDIR)\$(OUTNAME).c "

devicedllutilities.obj: $(DEVICEDLLUTILITIES_C) "$(HEADERS)\devicedllutilities.c" 
	"$(CC)" -c $(CFLAGS) "$(HEADERS)\devicedllutilities.c"




