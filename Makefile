All: bin/DataClean bin/CreateDict

# DataClean 目标
bin/DataClean: bin/DataClean.o
	g++ -o bin/DataClean bin/DataClean.o

bin/DataClean.o: src/DataClean.cc
	g++ -c src/DataClean.cc -o bin/DataClean.o -I include

# CreateDict 目标
bin/CreateDict: bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o
	g++ -o bin/CreateDict bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o

bin/CreateDict.o: src/CreateDict.cc
	g++ -c src/CreateDict.cc -o bin/CreateDict.o -I include

bin/DictProducer.o: src/DictProducer.cc include/DictProducer.h
	g++ -c src/DictProducer.cc -o bin/DictProducer.o -I include

bin/SplitToolCppJieba.o: src/SplitToolCppJieba.cc include/SplitToolCppJieba.h
	g++ -c src/SplitToolCppJieba.cc -o bin/SplitToolCppJieba.o -I include

bin/SplitTool.o: src/SplitTool.cc include/SplitTool.h
	g++ -c src/SplitTool.cc -o bin/SplitTool.o -I include

clean:
	rm -f bin/DataClean bin/CreateDict
	rm -f bin/*.o
	
rebuild: clean All

.PHONY: All clean rebuild