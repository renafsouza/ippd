#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>

#define byte unsigned char

using namespace std;

void compress(char* file_name, int width, int height, int frames, int block_size){
    ifstream file (file_name, ios::in|ios::binary|ios::ate);
    char extension[] = ".cmp";
    ofstream compressedFile (strcat(file_name,extension), ios::out|ios::binary|ios::ate);
    streampos readFileSize;
    // vector<pair<vector<pair<int, int>>,vector<pair<int, int>>>> matches;
    char * readMemBlock, * compressedMemBlock;

    if ( file.is_open() && compressedFile.is_open()) { // always check whether the file is open
        readFileSize = file.tellg();
        readMemBlock = new char [readFileSize];
        int writeFrameSize = width*height*8/(block_size*block_size)+(width*height*0.5);
        int readFrameSize = width*height*1.5;
        int writeFileSize = readFrameSize + writeFrameSize * (frames-1);
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
        for(int f=1;f<frames;++f){
            int readOffset = f*readFrameSize;
            int writeOffset = readFrameSize + (f-1)*writeFrameSize;

            for(int i=0;i<height;i+=block_size){
                for(int j=0;j<width;j+=block_size){
                    pair<int, int> bestMatch;
                    float bestDiffAvg = -1;
                    bool goodEnough = false;
                    // Iterate through each line of each frame
                    for(int i2 = 0;i2<height-block_size;++i2){
                        // Iterate through each column of each line
                        for(int j2 = 0;j2<width-block_size;++j2){
                            float diffAvg = 0;
                            // Iterate trough each line of an 8x8 block
                            for(int ib = 0;ib<block_size;ib++){
                                int lineOffset = (i+ib)*width;
                                // Iterate trough each column of an 8x8 block
                                for(int jb =0;jb<block_size;jb++){
                                    diffAvg += abs((int)readMemBlock[(readOffset+lineOffset)+j+jb] - (int)readMemBlock[(i2+ib)*width+j2+jb]);
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
            readOffset+=width*height;
            for(int i = 0;i<(int) width*height*0.5; i++){
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

void decompress(char* file_name, int width, int height, int frames, int block_size){
    ifstream compressedFile (file_name, ios::in|ios::binary|ios::ate);
    string newFileName = "";
    for (int i = 0;i<strlen(file_name)-4;i++)
        newFileName = newFileName+file_name[i];//file_name[i];
    ofstream decompressedFile (newFileName, ios::out|ios::binary|ios::ate);
    streampos readFileSize;
    streampos writeFileSize;
    char * readMemBlock, * writeMemBlock;
    if ( compressedFile.is_open() && decompressedFile.is_open()) { // always check whether the file is open
        readFileSize = compressedFile.tellg();
        int writeFrameSize = width*height*1.5;
        writeFileSize = frames*writeFrameSize;
        readMemBlock = new char [readFileSize];
        writeMemBlock = new char [writeFileSize];
        compressedFile.seekg (0, ios::beg);
        compressedFile.read(readMemBlock,readFileSize); // pipe file's content into stream
        compressedFile.close();

        // copy first frame
        int readFrameSize = width*height*8/(block_size*block_size)+width*height*0.5;

        for(int i = 0;i<writeFrameSize; i++){
            writeMemBlock[i] = readMemBlock[i];
        }

        for(int f = 1; f<frames; ++f){
            int writeOffset = f*writeFrameSize;
            int readOffset = writeFrameSize+(f-1)*readFrameSize;

            for(int i=0;i<height/block_size;i++){
                for(int j=0;j<width/block_size;j++){
                    int readLineWidth = 8*width/block_size;
                    int index = readOffset + i*readLineWidth + j*8;

                    char bytes1[4]{ readMemBlock[index+3], readMemBlock[index+2], readMemBlock[index+1], readMemBlock[index] };
                    char bytes2[4]{ readMemBlock[index+7], readMemBlock[index+6], readMemBlock[index+5], readMemBlock[index+4] };
                    short readBlockLine,readBlockColumn;
                    memcpy(&readBlockLine, bytes1, sizeof(int));
                    memcpy(&readBlockColumn, bytes2, sizeof(int));

                    for(int ib =0;ib<block_size;ib++){
                        for(int jb =0;jb<block_size;jb++){
                            
                            int lineOffset = (i*block_size+ib)*width,
                                columnOffset = j*block_size+jb;
                            int compressedFileLineOffset = (readBlockLine+ib)*width,
                                compressedFileColumnOffset = readBlockColumn+jb;

                            writeMemBlock[writeOffset+lineOffset+columnOffset] =
                                readMemBlock[compressedFileLineOffset+compressedFileColumnOffset];
                                
                        }
                    }
                }
            }
            writeOffset += width*height;
            readOffset+= width*height*8/(block_size*block_size);

            for(int i=0;i<(int)(width*height*0.5);i++){
                writeMemBlock[writeOffset+i] = readMemBlock[readOffset + i];
            }
        }

        decompressedFile.write (writeMemBlock, writeFileSize);
        decompressedFile.close();

    } else {
        cout << "Unable to open file";
    }
}
void printHelp(char* programName){
    cout<<"Uso: "<<programName<<" [Opções] [Alvo] [Frames] [Width] [Height] [?BlockSize]"<<endl;
    cout<<"Opções:"<<endl;
    cout<<"\t-c, --compress \t\tcomprime arquivo"<<endl;
    cout<<"\t-d, --decompress \tdescomprime arquivo"<<endl;
}
int main(int argc, char** argv){
    int block_size;
    if(argc>1 && (strcmp(argv[1],"-c") == 0 || strcmp(argv[1],"--compress") == 0)){
        if(argc<6){
            cout<<argv[0]<<": Número de argumentos insuficiente"<<endl;
            printHelp(argv[0]);
            return 0;
        }
        block_size = argc > 6?atoi(argv[6]):8;
        compress(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), block_size);
    }else if(argc>1 &&( strcmp(argv[1],"-d") == 0|| strcmp(argv[1],"--decompress") == 0)){
        if(argc<6){
            cout<<argv[0]<<": Número de argumentos insuficiente"<<endl;
            printHelp(argv[0]);
            return 0;
        }
        block_size = argc > 6?atoi(argv[6]):8;
        decompress(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), block_size);
    }else if(argc>1 && (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0)){
        printHelp(argv[0]);
    }else{
        cout<<argv[0]<<": Opção inválida '"<<argv[1]<<"'"<<endl;
        printHelp(argv[0]);
    }
    return 0;
}
