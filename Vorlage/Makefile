OBJ =	Zugvorlage.o StdAfx.o Metascript.o CRNE.o crashdumphandler.o ../EBase/Expression.o \
	../EBase/regexp.o ../EBase/Report.o ../EBase/ReportStream.o ../EBase/hierarchy.o \
	../EBase/StdAfx.o ../EBase/Utility.o ../EBase/Value.o ../EBase/Hash.o \
        ../EBase/charencoding.o

OS = $(shell uname -s)
CPU = $(shell uname -m)
# CC = @echo 'ERROR: Unsupported plattform: ' $(OS); exit 10 ; 

ifeq ($(OS),Linux)

BIN = vorlage
CPP = g++-3.3
# CC = @echo 'ERROR: Missing or unsupported MODE: ' $(MODE); exit 10 ; 

ifeq ($(MODE),debug)
  CC = $(CPP) -I/home/gulrak/development/boost_1_32_0
  CFLAGS = -g -static
  CPPFLAGS =
  LFLAGS = -static
else
ifeq ($(MODE),release)
  CC = $(CPP) -I/home/gulrak/development/boost_1_32_0 -O2
  CFLAGS = -static
  CPPFLAGS =
  LFLAGS = -static -Xlinker -M
else
ifeq ($(MODE),prerelease)
  CC = $(CPP) -I/home/gulrak/development/boost_1_32_0 -O2
  CFLAGS = -g -static
  CPPFLAGS =
  LFLAGS = -static
else
  $(error unsupported mode (use MODE=debug or MODE=release))
endif
endif
endif

else

BIN = vorlage.exe
CPP = D:/Development/MinGW/bin/g++
# CC = @echo 'ERROR: Missing or unsupported MODE: ' $(MODE); exit 10 ; 

ifeq ($(MODE),debug)
  CC = $(CPP) -ID:/Development/boost_1_29_0
  CFLAGS = -g -static -DSTATIC
  CPPFLAGS =
  LFLAGS = -static
else
ifeq ($(MODE),release)
  CC = $(CPP) -ID:/Development/boost_1_29_0 -Os
  CFLAGS = -static -DSTATIC
  CPPFLAGS =
  LFLAGS = -static
else
ifeq ($(MODE),prerelease)
  CC = $(CPP) -ID:/Development/boost_1_29_0 -Os
  CFLAGS = -g -static -DSTATIC
  CPPFLAGS =
  LFLAGS = -static
else
  $(error unsupported mode (use MODE=debug or MODE=release))
endif
endif
endif

#  $(error unsupported plattform: $(OS))

endif


%.o : %.cpp
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(BIN) : $(OBJ)
	$(CC) -o $(BIN) $(LFLAGS) $(OBJ) /usr/lib/libpcre.a

include depends.mak

.PHONY : prepare
prepare :
	-d:/Development/MingW/bin/strip --strip-unneeded $(BIN)
	-upx --best $(BIN)
	
.PHONY : clean
clean :
	-rm -f $(BIN) $(OBJ)

depend :
	$(CC) -M $(patsubst %.o,%.cpp,$(OBJ)) >depends.mak

vpp :
	$(CC) -o vpp $(CFLAGS) $(CPPFLAGS) VPP.cpp 
