
ifeq ($(shell uname -p),powerpc)
INC	 = bigendian
else 
ifeq ($(shell uname -p),sparc)
INC	 = bigendian
else
ifeq ($(shell uname -p),x86_64)
INC	 = littleendian
else
$(error "Unsupported architecture:" $(shell uname -p))
endif
endif
endif

CXX=g++
CXXFLAGS=-I$(INC) -I. -O2 -DNATIVE

all: fputest timesoftfloat

softfloat.o: $(INC)/milieu.h $(INC)/softfloat.h $(INC)/softfloat-specialize softfloat-macros softfloat.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ softfloat.cpp

timesoftfloat.o: $(INC)/milieu.h $(INC)/softfloat.h timesoftfloat.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ timesoftfloat.cpp

fputest.o: $(INC)/milieu.h $(INC)/softfloat.h fputest.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ fputest.cpp

timesoftfloat: softfloat.o timesoftfloat.o
	$(CXX) $(CXXFLAGS) -o $@ $?

pdouble: pdouble.cpp
	$(CXX) $(CXXFLAGS) -o $@ $?

fputest: softfloat.o fputest.o
	$(CXX) -o $@ $?

clean:
	-@rm -f fputest.o timesoftfloat.o fputest timesoftfloat
