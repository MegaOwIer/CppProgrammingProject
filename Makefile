CXX=`llvm-config --bindir`/clang
CXXFLAGS=`llvm-config --cxxflags` -shared -fPIC -std=c++17
LDFLAGS=`llvm-config --ldflags`
FILES=SimpleDataDependenceGraph.cpp MyPass.cpp Hash.cpp CountSupport.cpp DataDig.cpp RuleGenerator.cpp BugDetector.cpp
INCLUDES=-I./include

all: MyPass.so

debug: *.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILES) -O2 -o MyPass.so $(INCLUDES) -D_LOCAL_DEBUG

MyPass.so: *.cpp include/*.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILES) -O2 -o MyPass.so $(INCLUDES)

clean:
	rm MyPass.so