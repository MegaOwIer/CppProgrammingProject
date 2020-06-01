CXX=`llvm-config --bindir`/clang++
CXXFLAGS=`llvm-config --cxxflags` -shared -fPIC
LDFLAGS=`llvm-config --ldflags`
FILES=SimpleDataDependenceGraph.cpp MyPass.cpp Hash.cpp CountSupport.cpp
INCLUDES=-I./include

all: MyPass.so

MyPass.so: *.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILES) -O2 -o MyPass.so $(INCLUDES)

clean:
	rm MyPass.so