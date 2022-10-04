#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>

using namespace std;

const int HEIGHT = 360;
const int WIDTH = 640;
const int FRAMES = 120;


void loadFrames(byte *frames){
    ifstream file ("file.yuv", ios::in|ios::binary|ios::ate);
    streampos size;
    char * memBlock;
    if ( file.is_open() ) { // always check whether the file is open
        size = file.tellg();
        memBlock = new char [size];
        file.seekg (0, ios::beg);
        file.read(memBlock,size); // pipe file's content into stream
        file.close();

        
        for(int f=0;f<2;f++){
            for(int l =0;l<HEIGHT;l++){
                for(int col =0;col<WIDTH;col++){
                    frames[f*120+l*WIDTH+col] = (byte)memBlock[f*345600+l*WIDTH+col];
                }
            }
        }
        delete[] memBlock;
    }else{
        cout << "Unable to open file";
    }

}

vector<pair<int*,int*>> motion(byte* frames,int f1,int f2){
    vector<pair<int*,int*>> matches({});
    #pragma omp parallel for 
    for(int i=0;i<HEIGHT-8;i++){
        for(int j=0;j<WIDTH-8;j++){
            bool match = true;
            for(int i2=0;i2<HEIGHT-8;i2++){
                for(int j2=0;j2<WIDTH-8;j2++){
                    for(int section_i=0;section_i<8;section_i++){
                        for(int section_j=0;section_j<8;section_j++){
                            cout<<f2<<" "<<f2+(i2+section_i)*WIDTH+j2+section_j<<endl;
                            if(frames[f1+(i+section_i)*WIDTH+j+section_j] != frames[f2+(i2+section_i)*WIDTH+j2+section_j]){
                                match = false;
                                break;
                            }else{
                            }
                        }
                        if(!match) break;
                    }
                    if(match){
                        int p1[2] ={i,j};
                        int p2[2] ={i2,j2};
                        cout<<i<<" "<<j<<" "<<i2<<" "<<j2<<endl;
                        #pragma omp critical
                        matches.push_back(pair<int*,int*>(p1,p2));
                        break;
                    }
                }
                if(match){
                    break;
                }
            }
        }
    }
    return matches;
}

int main(){
    byte *frames3 = (byte *)malloc(FRAMES* HEIGHT * WIDTH * sizeof(byte));
    byte frames[1][HEIGHT][WIDTH];
    // byte frames2[120*HEIGHT*WIDTH];
    loadFrames(frames3);
    for(int i=1;i<FRAMES-1;i++){
        vector<pair<int*,int*>> matches;
        cout<<"match "<<i<<" "<<i+1<<"\n";
        matches = motion(frames3,i*HEIGHT*WIDTH,(i+1)*HEIGHT*WIDTH);
        // for(pair<int*,int*> match : matches) 
        //     cout << "match = " << (int)match.first[0] << " " << (int)match.second[0] << endl;
    }
    return 0;
}