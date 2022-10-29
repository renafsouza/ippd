#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <cstring>

#define byte unsigned char

using namespace std;

void compress(char* file_name, int width, int height, int frames, int block_size){
    ifstream readFile (file_name, ios::in|ios::binary|ios::ate);
    
    char extension[] = ".cmp";
    ofstream writeFile (strcat(file_name,extension), ios::out|ios::binary|ios::ate);

    char * readMemBlock, * compressedMemBlock;

    if ( readFile.is_open() && writeFile.is_open() ) {
        streampos readFileSize;
        readFileSize = readFile.tellg();
        int writeFrameSize = width*height*8/(block_size*block_size)+(width*height*0.5);
        int readFrameSize = width*height*1.5;
        int writeFileSize = readFrameSize + writeFrameSize * (frames-1);
        
        readMemBlock = new char [readFileSize];
        compressedMemBlock = new char [writeFileSize];

        readFile.seekg (0, ios::beg);
        readFile.read(readMemBlock, readFileSize);
        readFile.close();

        // Copies first frame
        for(int i = 0;i<readFrameSize; i++){
            compressedMemBlock[i] = readMemBlock[i];
        }

        // Loops through all frames except the first
        #pragma omp parallel for
        for(int f=1;f<frames;++f){
            int readOffset = f*readFrameSize;
            int writeOffset = readFrameSize + (f-1)*writeFrameSize;

            // Loops through every (block_size x block_size) block in the frame
            for(int i=0; i<height; i+=block_size){
                for(int j=0; j<width; j+=block_size){
                    pair<int, int> bestMatch;
                    float bestDiffAvg = -1;
                    
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
                        }
                    }
                    
                    // Write the best matches in the vector
                    compressedMemBlock[writeOffset++] = bestMatch.first;
                    compressedMemBlock[writeOffset++] = bestMatch.first>>8;
                    compressedMemBlock[writeOffset++] = bestMatch.first>>16;
                    compressedMemBlock[writeOffset++] = bestMatch.first>>24;
                    compressedMemBlock[writeOffset++] = bestMatch.second;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>8;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>16;
                    compressedMemBlock[writeOffset++] = bestMatch.second>>24;
                }
            }

            readOffset += width*height;

            // Copies the chrominance values
            for(int i = 0;i<(int) width*height*0.5; i++){
                compressedMemBlock[writeOffset+i] = readMemBlock[readOffset+i];
            }
        }

        writeFile.write (compressedMemBlock, writeFileSize);
        writeFile.close();
    } else {
        cout << "Unable to open file";
    }
}

void decompress(char* file_name, int width, int height, int frames, int block_size){
    ifstream compressedFile (file_name, ios::in|ios::binary|ios::ate);
    string newFileName = "";
    for (int i = 0;i<strlen(file_name)-4;i++)
        newFileName = newFileName+file_name[i];
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
        compressedFile.read(readMemBlock,readFileSize);
        compressedFile.close();

        // Copies first frame
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
                    int bestMatchOffset = readOffset + i*readLineWidth + j*8;


                    // Reads the best matched block's line and column from the compressed file
                    char bestMatchLineByteArr[4]{ 
                        readMemBlock[bestMatchOffset],
                        readMemBlock[bestMatchOffset+1],
                        readMemBlock[bestMatchOffset+2],
                        readMemBlock[bestMatchOffset+3]
                    };
                    char bestMatchColumnByteArr[4]{ 
                        readMemBlock[bestMatchOffset+4],
                        readMemBlock[bestMatchOffset+5],
                        readMemBlock[bestMatchOffset+6],
                        readMemBlock[bestMatchOffset+7]
                    };

                    short bestMatchLine,bestMatchColumn;
                    memcpy(&bestMatchLine, bestMatchLineByteArr, sizeof(int));
                    memcpy(&bestMatchColumn, bestMatchColumnByteArr, sizeof(int));

                    // Writes the best matched block into the new file
                    for(int ib =0;ib<block_size;ib++){
                        for(int jb =0;jb<block_size;jb++){
                            
                            int lineOffset = (i*block_size+ib)*width,
                                columnOffset = j*block_size+jb;
                            int bestMatchLineOffset = (bestMatchLine+ib)*width,
                                bestMatchColumnOffset = bestMatchColumn+jb;

                            writeMemBlock[ writeOffset+lineOffset+columnOffset ] =
                                readMemBlock[ bestMatchLineOffset+bestMatchColumnOffset ];
                                
                        }
                    }
                }
            }
            writeOffset += width*height;
            readOffset+= width*height*8/(block_size*block_size);

            // Copies the chrominance values
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
    if( argc>1 && (strcmp(argv[1],"-c") == 0 || strcmp(argv[1],"--compress") == 0)){
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
