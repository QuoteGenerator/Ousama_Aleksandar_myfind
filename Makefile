myfind: main.cpp
	g++ -Wall -Wextra -std=c++17 main.cpp -o myfind

clean: 
	rm -f myfind

.PHONY: all clean 