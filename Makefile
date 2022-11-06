all: decompress compress
compress:
	mpic++ compress.cpp -fopenmp -lpthread -o compress
decompress:
	g++ decompress.cpp -lpthread -fopenmp -o decompress