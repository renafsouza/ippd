#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

#define byte unsigned char

using namespace std;

const int WIDTH = 640, FRAMES = 2, HEIGHT = 360, BLOCK_SIZE = 8, THRESHOLD = 5, MAX_BLOCK_MATCHES = 1;

int main(){
    ifstream file ("file.yuv", ios::in|ios::binary|ios::ate);
    ofstream newFile ("newFile.yuv", ios::in|ios::binary|ios::ate);
    streampos size;
    vector<pair<vector<pair<int, int>>,vector<pair<int, int>>>> matches;
    char * memBlock;

    if ( file.is_open() && newFile.is_open()) { // always check whether the file is open
        size = file.tellg();
        memBlock = new char [size];
        file.seekg (0, ios::beg);
        file.read(memBlock,size); // pipe file's content into stream
        file.close();
        byte frame1[HEIGHT][WIDTH];

        #pragma omp parallel for
        // Iterate through each frame
        for(int f=1;f<FRAMES;++f){
            vector<pair<int, int>> Ra, Rv;
            // Iterate through each line of each frame
            for(int i=0;i<HEIGHT-8;++i){
                // Iterate through each column of each line
                for(int j=0;j<WIDTH-8;++j){
                    int blockMatches = 0;
                    // Iterate through each line of each frame
                    for(int i2=0;i2<HEIGHT-8;++i2){
                        // Iterate through each column of each line
                        for(int j2 =0;j2<WIDTH-8;++j2){
                            bool match = true;
                            // Iterate trough each line of an 8x8 block
                            for(int ib =0;ib<BLOCK_SIZE;ib++){
                                // Iterate trough each column of an 8x8 block
                                for(int jb =0;jb<BLOCK_SIZE;jb++){
                                    if(abs(memBlock[(i+ib*WIDTH)+j+jb] - (byte)memBlock[f*345600+(i2+ib)*WIDTH+j2+jb]) > THRESHOLD){
                                        match = false;
                                        break;
                                    } 
                                }
                                if(!match) break;
                            }
                            if(match){
                                Ra.push_back(pair<int,int>(j,i));
                                Rv.push_back(pair<int,int>(j2,i2));
                                ++blockMatches;
                                j2+=8;
                                for(int ib = 0;ib<BLOCK_SIZE;ib++){
                                    for(int jb =0;jb<BLOCK_SIZE;jb++){
                                        memBlock[f*345600+(i2+ib)*WIDTH+j2+jb] = memBlock[(i+ib*WIDTH)+j+jb];
                                    }
                                }
                                break;
                            }
                        }
                        if(blockMatches >= MAX_BLOCK_MATCHES) break;
                    }
                    if(blockMatches > 0) j+=8;
                }
            }
            matches.push_back(pair<vector<pair<int, int>>,vector<pair<int, int>>>(Ra,Rv));
        }

        newFile.write (memBlock, size);
        newFile.close();
    } else {
        cout << "Unable to open file";
        return 1;
    }
    return 0;
}
