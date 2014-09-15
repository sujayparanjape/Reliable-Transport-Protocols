UNAME = $(shell uname)
ifeq ($(UNAME), SunOS) # Sun OS
MY_LIBS = -lresolv -lsocket -lnsl
endif
ifeq ($(UNAME), Linux) # Linux
MY_LIBS = -lresolv -lnsl -lpthread
endif
ifeq ($(UNAME), Darwin) # Mac OS
MY_LIBS =
endif

CC := g++

all: abt.o gbn.o sr.o

abt.o: abt_CustomVariables.h abt.cpp
	${CC} abt.cpp -o abt

gbn.o: gbn_CustomVariables.h gbn.cpp
	${CC} gbn.cpp -o gbn

sr.o: sr_CustomVariables.h sr.cpp
	${CC} sr.cpp -o sr

##==========================================================================
clean:
	@- $(RM) abt gbn sr
	@- echo “Data Cleansing Done.Ready to Compile”
