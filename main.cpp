#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

#define byte unsigned char

using namespace std;

const int HEIGHT = 360;
const int WIDTH = 640;
const int FRAMES = 120;

int main(){
    ifstream file ("file.yuv", ios::in|ios::binary|ios::ate);
    streampos size;
    char * memBlock;
    if ( file.is_open() ) { // always check whether the file is open
        size = file.tellg();
        memBlock = new char [size];
        file.seekg (0, ios::beg);
        file.read(memBlock,size); // pipe file's content into stream
        file.close();

        
        byte frame1[HEIGHT][WIDTH];
        for(int i =0;i<HEIGHT;i++){
            for(int j =0;j<WIDTH;j++){
                frame1[i][j] = (byte)memBlock[i*WIDTH+j];
            }
        }
        #pragma omp parallel for
        for(int f=1;f<FRAMES;f++){
            // primeiro laço
            for(int i =0;i<HEIGHT-8;i++){
                for(int j =0;j<WIDTH-8;j++){
                    // segundo laço
                    for(int i2 =0;i2<HEIGHT-8;i2++){
                        for(int j2 =0;j2<WIDTH-8;j2++){
                            // laço do bloco
                            bool match = true;
                            for(int ib =0;ib<8;ib++){
                                for(int jb =0;jb<8;jb++){
                                    if(abs(frame1[i+ib][j+jb] - (byte)memBlock[f*345600+(i2+ib)*WIDTH+j2+jb]) >5){
                                        match = false;
                                        break;
                                    } 
                                }
                                if(!match) break;
                            }
                            if(match){
                                cout<<"f:"<<f<<" i: "<<i<<" j: "<<j<<" i2: "<<i2<<" j2: "<<j2<<endl;
                            }
                        }
                    }
                    
                }
            }
        }
        delete[] memBlock;
    }else{
        cout << "Unable to open file";
    }
    return 0;
}