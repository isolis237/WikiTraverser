#compile program (./wikiTraverser to run)
wikiTraverser: adjacency_list.o digraph.o directed_edge.o node.o dfs.o dijkstra.o main.o
	g++ -std=c++11 adjacency_list.o digraph.o directed_edge.o node.o dfs.o dijkstra.o main.o -o wikiTraverser
	@echo "\n****************************************************************************************************\n"
	@echo "Enter start and end links when running (Ex. ./wiki_traverser Military_dictatorship Michael_Jordan)"
	@echo "Links can be found under data/links.tsv"
	@echo "\n****************************************************************************************************\n"

adjacency_list.o: main/src/adjacency_list.cpp
	g++ -std=c++11 -c ./main/src/adjacency_list.cpp

digraph.o: main/src/digraph.cpp
	g++ -std=c++11 -c ./main/src/digraph.cpp

directed_edge.o: main/src/directed_edge.cpp
	g++ -std=c++11 -c ./main/src/directed_edge.cpp

node.o: main/src/node.cpp
	g++ -std=c++11 -c ./main/src/node.cpp

dfs.o: main/src/dfs.cpp
	g++ -std=c++11 -c ./main/src/dfs.cpp

dijkstra.o: main/src/dijkstra.cpp
	g++ -std=c++11 -c ./main/src/dijkstra.cpp

main.o: main.cpp
	g++ -std=c++11 -c main.cpp

#compile tests
# tests: test_adjacency_lists.o test_dfs.o test_digraph.o test_dijkstra.o test_directed_edges.o test_node.o test.o
# 	g++ test_adjacency_lists.o test_dfs.o test_digraph.o test_dijkstra.o test_directed_edges.o test_node.o test.o -o tests

# test_adjacency_lists.o: ./main/tests/test_adjacency_lists.cpp
# 	g++ -std=c++11 -c ./main/tests/test_adjacency_lists.cpp

# test_dfs.o: ./main/tests/test_dfs.cpp
# 	g++ -std=c++11 -c ./main/tests/test_dfs.cpp

# test_digraph.o: ./main/tests/test_digraph.cpp
# 	g++ -std=c++11 -c ./main/tests/test_digraph.cpp

# test_dijkstra.o: ./main/tests/test_dijkstra.cpp
# 	g++ -std=c++11 -c ./main/tests/test_dijkstra.cpp

# test_directed_edges.o: ./main/tests/test_directed_edges.cpp
# 	g++ -std=c++11 -c ./main/tests/test_directed_edges.cpp

# test_node.o: ./main/tests/test_node.cpp
# 	g++ -std=c++11 -c ./main/tests/test_node.cpp

# test.o: ./main/tests/test.cpp
# 	g++ -std=c++11 -c ./main/tests/test.cpp


#clear all object files
clean:
	rm *.o wiki_traverser