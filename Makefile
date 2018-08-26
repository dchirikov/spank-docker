all:
	g++ -std=c++11 -fPIC -shared spank-docker.cpp -o spank-docker.so

clean:
	rm -f *.o *.so
