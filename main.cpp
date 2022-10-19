#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

#define byte unsigned char

using namespace std;

const int WIDTH = 640, FRAMES = 120, HEIGHT = 360, BLOCK_SIZE = 8;

int main(){
    ifstream file ("file.yuv", ios::in|ios::binary|ios::ate);
    ofstream newFile ("newFile.yuv", ios::in|ios::binary|ios::ate);
    streampos size;
    // vector<pair<vector<pair<int, int>>,vector<pair<int, int>>>> matches;
    char * memBlock;

    if ( file.is_open() && newFile.is_open()) { // always check whether the file is open
        size = file.tellg();
        memBlock = new char [size];
        file.seekg (0, ios::beg);
        file.read(memBlock,size); // pipe file's content into stream
        file.close();
        byte frame1[HEIGHT][WIDTH];

        // Iterate through each frame
        #pragma omp parallel for
        for(int f=1;f<FRAMES;++f){
            vector<pair<int, int>> Ra, Rv;
            for(int i=0;i<HEIGHT-BLOCK_SIZE;i+=BLOCK_SIZE){
                for(int j=0;j<WIDTH-BLOCK_SIZE;j+=BLOCK_SIZE){
                    pair<int, int> bestMatch;
                    float bestDiffAvg = -1;
                    // Iterate through each line of each frame
                    for(int i2=0;i2<HEIGHT-BLOCK_SIZE;++i2){
                        // Iterate through each column of each line
                        for(int j2 =0;j2<WIDTH-BLOCK_SIZE;++j2){
                            float diffAvg = 0;
                            // Iterate trough each line of an 8x8 block
                            for(int ib =0;ib<BLOCK_SIZE;ib++){
                                // Iterate trough each column of an 8x8 block
                                for(int jb =0;jb<BLOCK_SIZE;jb++){
                                    diffAvg += abs((int)memBlock[(f*345600+(i+ib)*WIDTH)+j+jb] - (int)memBlock[(i2+ib)*WIDTH+j2+jb]);
                                }
                            }
                            diffAvg = diffAvg / 64;
                            if(diffAvg<bestDiffAvg || bestDiffAvg==-1){
                                bestMatch = pair<int, int>(i2,j2);
                                bestDiffAvg = diffAvg;
                            }
                        }
                    }
                    // Ra.push_back(pair<int,int>(j,i));
                    // Rv.push_back(bestMatch);
                    for(int ib =0;ib<BLOCK_SIZE;ib++){
                        // Iterate trough each column of an 8x8 block
                        for(int jb =0;jb<BLOCK_SIZE;jb++){
                            memBlock[f*345600+(i+ib)*WIDTH+j+jb] = memBlock[(bestMatch.first+ib)*WIDTH+bestMatch.second+jb];
                        }
                    }
                }
            }
            // matches.push_back(pair<vector<pair<int, int>>,vector<pair<int, int>>>(Ra,Rv));
        }

        newFile.write (memBlock, size);
        newFile.close();
    } else {
        cout << "Unable to open file";
        return 1;
    }
    return 0;
}
