#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>
using namespace std;

void compress(char* file_name, int width, int height, int frames, int block_size){
    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    ifstream readFile (file_name, ios::in|ios::binary|ios::ate);
    
    char extension[] = ".cmp";
    ofstream writeFile (strcat(file_name,extension), ios::out|ios::binary|ios::ate);

    char * readMemBlock, * sendMemBlock, * writeMemBlock;
    if ( readFile.is_open() && writeFile.is_open() ) {
        streampos readFileSize;
        readFileSize = readFile.tellg();
        int writeFrameSize = width*height*8/(block_size*block_size)+(width*height*0.5);
        int readFrameSize = width*height*1.5;
        int writeFileSize = readFrameSize + writeFrameSize * (frames-1);
        
        readMemBlock = new char [readFileSize];
        sendMemBlock = new char [writeFileSize];
        writeMemBlock = new char [writeFileSize];

        readFile.seekg (0, ios::beg);
        readFile.read(readMemBlock, readFileSize);
        readFile.close();


        int rangeSize = floor((frames-1)/world_size);
        int rangeStart = rangeSize*world_rank+1;
        int rangeEnd = world_rank==(world_size-1)?frames:rangeStart+rangeSize;
        // Loops through all frames except the first
        #pragma omp parallel for
        for(int f=rangeStart;f<rangeEnd;++f){
            int readOffset = f*readFrameSize;
            int writeOffset = (f-rangeStart)*writeFrameSize;

            // Loops through every (block_size x block_size) block in the frame
            for(int i=0; i<height; i+=block_size){
                for(int j=0; j<width; j+=block_size){
                    pair<int, int> bestMatch;
                    float bestDiffAvg = -1;
                    // bool goodEnough = false;
                    
                    // Finds the (block_size x block_size) block that best matches the block in the original frame
                    for(int i2 = 0;i2<height-block_size;++i2){
                        for(int j2 = 0;j2<width-block_size;++j2){
                            float diffAvg = 0;
                            for(int ib = 0;ib<block_size;ib++){
                                int lineOffset = (i+ib)*width;
                                for(int jb =0;jb<block_size;jb++){
                                    diffAvg += abs((int)readMemBlock[(readOffset+lineOffset)+j+jb] - (int)readMemBlock[(i2+ib)*width+j2+jb]);
                                }
                            }
                            diffAvg = diffAvg / 64;
                            if(diffAvg<bestDiffAvg || bestDiffAvg==-1){
                                bestMatch = pair<int, int>(i2,j2);
                                bestDiffAvg = diffAvg;
                            }
                            // goodEnough = true;
                            // if(goodEnough) break;
                            // break;
                        }
                        // if(goodEnough) break;
                        // break;
                    }
                    
                    // Write the best matches in the vector
                    sendMemBlock[writeOffset++] = bestMatch.first;
                    sendMemBlock[writeOffset++] = bestMatch.first>>8;
                    sendMemBlock[writeOffset++] = bestMatch.first>>16;
                    sendMemBlock[writeOffset++] = bestMatch.first>>24;
                    sendMemBlock[writeOffset++] = bestMatch.second;
                    sendMemBlock[writeOffset++] = bestMatch.second>>8;
                    sendMemBlock[writeOffset++] = bestMatch.second>>16;
                    sendMemBlock[writeOffset++] = bestMatch.second>>24;
                }
            }

            readOffset += width*height;

            // Copies the chrominance values
            for(int i = 0;i<(int) width*height*0.5; i++){
                sendMemBlock[writeOffset+i] = readMemBlock[readOffset+i];
            }
        }
        cout<<writeFileSize<<" "<< writeFrameSize*(frames-1)+readFrameSize<<endl;
        // cout<< (rangeEnd-rangeStart)*writeFileSize << " "<< p << " "<< writeFileSize*(frames-1)<<" "<<writeFileSize<< endl;

        int recvCounts[world_size], recvDispls[world_size];
        if(world_rank == 0){
            for(int i = 0; i<world_size; i++){
                int rangeSize = floor((frames-1)/world_size);
                int rangeStart = rangeSize*i;
                recvCounts[i] = writeFrameSize*rangeSize;
                recvDispls[i] = rangeStart*writeFrameSize;
                cout<<recvDispls[i]<<endl;
            }

        }
        MPI_Gatherv(
            sendMemBlock, 
            rangeSize*writeFrameSize, 
            MPI_CHAR, 
            writeMemBlock+readFrameSize,
            recvCounts, 
            recvDispls, 
            MPI_CHAR, 
            0, 
            MPI_COMM_WORLD
        );
        // Copies first frame
        if(world_rank == 0){
            for(int i = 0; i<readFrameSize; i++){
                writeMemBlock[i] = readMemBlock[i];
            }
            writeFile.write (writeMemBlock, writeFileSize);
            writeFile.close();
        }
    } else {
        cout << "Unable to open file";
    }
}
void printHelp(char* programName){
    // Get the rank of the process
    cout<<"Uso: "<<programName<<" [Alvo] [Frames] [Width] [Height] [?BlockSize]"<<endl;
}
int main(int argc, char** argv){
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int block_size;
    if(argc>1 && (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0)){
        if(world_rank == 0) printHelp(argv[0]);
    }else if( argc ){
        if(argc<5){
            if(world_rank == 0){
                cout<<argv[0]<<": Número de argumentos insuficiente"<<endl;
                printHelp(argv[0]);
            }
            return 0;
        }
        block_size = argc > 6?atoi(argv[6]):8;
        compress(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), block_size);
    } 
    MPI_Finalize();
    return 0;
}