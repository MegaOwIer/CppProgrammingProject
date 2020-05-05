CXX=`llvm-config --bindir`/clang
CXXFLAGS=`llvm-config --cxxflags` -shared -fPIC
LDFLAGS=`llvm-config --ldflags`
FILES=SimpleDataDependenceGraph.cpp MyPass.cpp Hash.cpp

all: MyPass.so

MyPass.so: *.cpp *.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILES) -o MyPass.so

clean:
	rm MyPass.so