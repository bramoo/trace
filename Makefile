CXX = clang++ -c
CXXFLAGS = -MMD

LINKER = clang++ -o
LFLAGS =

all: bin/trace

bin/trace: obj/trace.o
	$(LINKER) $(LFLAGS) bin/trace obj/trace.o
	@echo "Linking complete!"

obj/trace.o: src/trace.cpp Makefile
	$(CXX) $(CXXFLAGS) src/trace.cpp -o obj/trace.o
	@echo "Compiled successfully!"

.PHONY: clean
clean:
	@rm obj/* bin/*
	@echo "Cleanup complete!"

-include obj/trace.d
$(shell mkdir bin obj)
