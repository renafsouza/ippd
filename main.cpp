#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>

#define byte unsigned char

using namespace std;

const int WIDTH = 640, FRAMES = 10, HEIGHT = 360, BLOCK_SIZE = 8;

void compress(){
    ifstream file ("file.yuv", ios::in|ios::binary|ios::ate);
    ofstream compressedFile ("compressed.yuv", ios::out|ios::binary|ios::ate);
    streampos readFileSize;
    // vector<pair<vector<pair<int, int>>,vector<pair<int, int>>>> matches;
    char * readMemBlock, * compressedMemBlock;

    if ( file.is_open() && compressedFile.is_open()) { // always check whether the file is open
        readFileSize = file.tellg();
        readMemBlock = new char [readFileSize];
        int writeFrameSize = WIDTH*HEIGHT*8/(BLOCK_SIZE*BLOCK_SIZE)+(WIDTH*HEIGHT*0.5);
        int readFrameSize = WIDTH*HEIGHT*1.5;
        int writeFileSize = readFrameSize + writeFrameSize * (FRAMES-1);
        compressedMemBlock = new char [writeFileSize];
        for(int i = 0;i<writeFileSize;i++){
            compressedMemBlock[i] = 0;
        }

        file.seekg (0, ios::beg);
        file.read(readMemBlock, readFileSize); // pipe file's content into stream
        file.close();

        // Iterate through each frame except the first
        int frameCount = 0;

        // copy first frame
        for(int i = 0;i<readFrameSize; i++){
            compressedMemBlock[i] = readMemBlock[i];
        }

        #pragma omp parallel for
        for(int f=1;f<FRAMES;++f){
            int readOffset = f*readFrameSize;
            int writeOffset = readFrameSize + (f-1)*writeFrameSize;

            for(int i=0;i<HEIGHT;i+=BLOCK_SIZE){
                for(int j=0;j<WIDTH;j+=BLOCK_SIZE){
                    pair<int, int> bestMatch;
                    float bestDiffAvg = -1;
                    bool goodEnough = false;
                    // Iterate through each line of each frame
                    for(int i2 = 0;i2<HEIGHT-BLOCK_SIZE;++i2){
                        // Iterate through each column of each line
                        for(int j2 = 0;j2<WIDTH-BLOCK_SIZE;++j2){
                            float diffAvg = 0;
                            // Iterate trough each line of an 8x8 block
                            for(int ib = 0;ib<BLOCK_SIZE;ib++){
                                int lineOffset = (i+ib)*WIDTH;
                                // Iterate trough each column of an 8x8 block
                                for(int jb =0;jb<BLOCK_SIZE;jb++){
                                    diffAvg += abs((int)readMemBlock[(readOffset+lineOffset)+j+jb] - (int)readMemBlock[(i2+ib)*WIDTH+j2+jb]);
                                }
                            }
                            diffAvg = diffAvg / 64;
                            if(diffAvg<bestDiffAvg || bestDiffAvg==-1){
                                bestMatch = pair<int, int>(i2,j2);
                                bestDiffAvg = diffAvg;
                            }
                        }
                    }
                    
                    compressedMemBlock[writeOffset++] = bestMatch.first>>24;
                    compressedMemBlock[writeOffset++] = bestMatch.first>>16;
                    compressedMemBlock[writeOffset++] = bestMatch.first>>8;
                    compressedMemBlock[writeOffset++] = bestMatch.first;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>24;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>16;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>8;
                    compressedMemBlock[writeOffset++] = bestMatch.second;
                }
            }
            readOffset+=WIDTH*HEIGHT;
            for(int i = 0;i<(int) WIDTH*HEIGHT*0.5; i++){
                compressedMemBlock[writeOffset+i] = readMemBlock[readOffset+i];
            }
            cout<<"f: "<<frameCount++<<endl;
        }

        compressedFile.write (compressedMemBlock, writeFileSize);
        compressedFile.close();
        // ifstream file ("compressed.yuv", ios::in|ios::binary|ios::ate);
    } else {
        cout << "Unable to open file";
    }
}

void decompress(){
    ifstream compressedFile ("compressedc.yuv", ios::in|ios::binary|ios::ate);
    ofstream decompressedFile ("decompressed.yuv", ios::out|ios::binary|ios::ate);
    streampos readFileSize;
    streampos writeFileSize;
    char * readMemBlock, * writeMemBlock;
    if ( compressedFile.is_open() && decompressedFile.is_open()) { // always check whether the file is open
        readFileSize = compressedFile.tellg();
        int writeFrameSize = WIDTH*HEIGHT*1.5;
        writeFileSize = FRAMES*writeFrameSize;
        readMemBlock = new char [readFileSize];
        writeMemBlock = new char [writeFileSize];
        compressedFile.seekg (0, ios::beg);
        compressedFile.read(readMemBlock,readFileSize); // pipe file's content into stream
        compressedFile.close();

        // copy first frame
        int readFrameSize = WIDTH*HEIGHT*8/(BLOCK_SIZE*BLOCK_SIZE)+WIDTH*HEIGHT*0.5;

        for(int i = 0;i<writeFrameSize; i++){
            writeMemBlock[i] = readMemBlock[i];
        }

        for(int f = 1; f<FRAMES; ++f){
            int writeOffset = f*writeFrameSize;
            int readOffset = writeFrameSize+(f-1)*readFrameSize;

            for(int i=0;i<HEIGHT/BLOCK_SIZE;i++){
                for(int j=0;j<WIDTH/BLOCK_SIZE;j++){
                    int readLineWidth = 8*WIDTH/BLOCK_SIZE;
                    int index = readOffset + i*readLineWidth + j*8;

                    unsigned char bytes1[4]{ readMemBlock[index+3], readMemBlock[index+2], readMemBlock[index+1], readMemBlock[index] };
                    unsigned char bytes2[4]{ readMemBlock[index+7], readMemBlock[index+6], readMemBlock[index+5], readMemBlock[index+4] };
                    short readBlockLine,readBlockColumn;
                    memcpy(&readBlockLine, bytes1, sizeof(int));
                    memcpy(&readBlockColumn, bytes2, sizeof(int));
                    cout<<readBlockColumn<<endl;

                    for(int ib =0;ib<BLOCK_SIZE;ib++){
                        for(int jb =0;jb<BLOCK_SIZE;jb++){
                            
                            int lineOffset = (i*BLOCK_SIZE+ib)*WIDTH,
                                columnOffset = j*BLOCK_SIZE+jb;
                            int compressedFileLineOffset = (readBlockLine+ib)*WIDTH,
                                compressedFileColumnOffset = readBlockColumn+jb;

                            writeMemBlock[writeOffset+lineOffset+columnOffset] =
                                readMemBlock[compressedFileLineOffset+compressedFileColumnOffset];
                                
                        }
                    }
                }
            }
            writeOffset += WIDTH*HEIGHT;
            readOffset+= WIDTH*HEIGHT*8/(BLOCK_SIZE*BLOCK_SIZE);

            for(int i=0;i<(int)(WIDTH*HEIGHT*0.5);i++){
                writeMemBlock[writeOffset+i] = readMemBlock[readOffset + i];
            }
        }

        decompressedFile.write (writeMemBlock, writeFileSize);
        decompressedFile.close();

    } else {
        cout << "Unable to open file";
    }
}

int main(int argc, char** argv){
    // compress();
    decompress();
    return 0;
}
