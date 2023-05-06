#import <vector>
#import <string>

#import "MultiTrackQTMovieUtils.h"

namespace MultiTrackQTMovie {

    class Parser {
        
        private:
        
            unsigned int _tracks = 0;
            std::vector<TrackInfo *> _info;
            std::vector<unsigned int> _totalFrames;
            std::vector<std::pair<u64,unsigned int> *> _frames;
            std::vector<std::vector<unsigned char *>> _ps;
            std::vector<unsigned int> _traks;
            
#ifdef EMSCRIPTEN
    
            unsigned char *_bytes;
            u64 _length;
#else 
        
            NSFileHandle *_handle;
            std::vector<std::vector<NSData *>> _PS;
#endif

            void reset() {
                this->_info.clear();
                this->_info.shrink_to_fit();
                this->_totalFrames.clear();
                this->_totalFrames.shrink_to_fit();
                this->_frames.clear();
                this->_frames.shrink_to_fit();
            }

            void clear() {
                
                for(int n=0; n<this->_frames.size(); n++) {
                    delete[] this->_frames[n];
                }

                this->reset();
                
                for(int n=0; n<this->_tracks; n++) {
                    if(this->_ps[n][2]) delete[] this->_ps[n][2];
                    if(this->_ps[n][1]) delete[] this->_ps[n][1];
                    if(this->_ps[n][0]) delete[] this->_ps[n][0];
                    
                    this->_ps[n][2] = nullptr;
                    this->_ps[n][1] = nullptr;
                    this->_ps[n][0] = nullptr;
                    
#ifndef EMSCRIPTEN
                    this->_PS[n][2] = nil;
                    this->_PS[n][1] = nil;
                    this->_PS[n][0] = nil;
#endif
                }
                
                this->_ps.clear();
                this->_ps.shrink_to_fit();
                
#ifndef EMSCRIPTEN
                this->_PS.clear();
                this->_PS.shrink_to_fit();
#endif
                
                this->_tracks = 0;

            }
        
        public:
            
            unsigned int tracks() {
                return this->_tracks;
            }
            
            unsigned int length(unsigned int trackid) {
                if(trackid<this->_totalFrames.size()) {
                    return this->_totalFrames[trackid];
                }
                return 0;
            };
            
            unsigned short width(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).width;
                }
                return 0;
            };
            
            unsigned short height(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).height;
                }
                return 0;
            };
            
            unsigned int depth(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).depth;
                }
                return 0;
            };
            
            std::string codec(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).codec;
                }
                return "";
            };
            
            unsigned int FPS(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).fps;
                }
                return 0;
            };
        
            unsigned int atom(std::string str) {
                
#ifndef EMSCRIPTEN
                assert(str.length()==4);
#endif
                unsigned char *key =(unsigned char *)str.c_str();
                return key[0]<<24|key[1]<<16|key[2]<<8|key[3];
            }
            
            std::string atom(unsigned int v) {
                char str[5];
                str[0] = (v>>24)&0xFF;
                str[1] = (v>>16)&0xFF;
                str[2] = (v>>8)&0xFF;
                str[3] = (v)&0xFF;
                str[4] = '\0';
                
                return str;
            }
        
            void parseTrack(unsigned char *moov, u64 len) {
                
                std::vector<unsigned int> track;
                
                for(int k=0; k<len; k++) {
                    for(int n=0; n<this->_traks.size(); n++) {
                        if(U32(moov+k)==this->_traks[n]) {
                            track.push_back(k);
                        }
                    }
                }
                
                this->_tracks = (unsigned int)track.size();
                
                unsigned int TimeScale = 0;
                
                for(int k=0; k<len-(4*4); k++) {
                    if(U32(moov+k)==this->atom("mvhd")) {
                        TimeScale = U32(moov+k+(4*4));
                        break;
                    }
                }
                
                std::vector<TrackInfo> info;
                                
                for(int n=0; n<this->_tracks; n++) {
                    
                    this->_ps.push_back({nullptr,nullptr,nullptr});

#ifndef EMSCRIPTEN
                    this->_PS.push_back({nil,nil,nil});
#endif
                    
                    TrackInfo *info = nullptr;
                    
                    unsigned int begin = track[n];
                    unsigned int end = begin + U32(moov+track[n]-4)-4;
                    
                    unsigned int info_offset[4];
                    info_offset[0] = 4*4; 
                    info_offset[1] = info_offset[0]+4+6+2+2+2+4+4+4; 
                    info_offset[2] = info_offset[1]+2; 
                    info_offset[3] = info_offset[2]+4+4+4+2+32+2; 
                    
                    for(int k=begin; k<end-info_offset[3]; k++) {
                        if(U32(moov+k)==this->atom("stsd")) {
                            info = new TrackInfo;
                            info->codec = atom(U32(moov+k+info_offset[0]));
                            info->width  = U16(moov+k+info_offset[1]);
                            info->height = U16(moov+k+info_offset[2]);
                            info->depth  = U16(moov+k+info_offset[3]);
                            break;
                        }
                    }
                    
                    if(info) {
                        
                        if(info->codec=="hvc1") {
                            int offset = 4+1+1+4+6+1+2+1+1+1+1+2+1;
                            for(int k=begin; k<end-4-offset; k++) {
                                if(U32(moov+k)==atom("hvcC")) {
                                    unsigned char *p = moov+k+offset;
                                    unsigned char num = *p++;
                                    if(num==3) {
                                        while(num--) {
                                            //unsigned char type = (*p++)&0x7F;
                                            p++;
                                            p+=2;
                                            unsigned short size = U16(p);
                                            p+=2;
                                            this->_ps[n][num] = new unsigned char[size+4];
                                            this->_ps[n][num][0] = 0;
                                            this->_ps[n][num][1] = 0;
                                            memcpy(this->_ps[n][num]+2,p-2,size+2);
#ifndef EMSCRIPTEN
                                            
                                            this->_PS[n][num] = [[NSData alloc] initWithBytes:this->_ps[n][num] length:(size+4)];
#endif
                                            
                                            if(num) p+=size;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        else if(info->codec=="avc1") {
                            int offset = 6;
                            for(int k=begin; k<end-4-offset; k++) {
                                if(U32(moov+k)==atom("avcC")) {
                                    unsigned char *p = moov+k+offset;
                                    unsigned short size = U16(p);
                                    p+=2;
                                    this->_ps[n][1] = new unsigned char[size+4];
                                    this->_ps[n][1][0] = 0;
                                    this->_ps[n][1][1] = 0;
                                    memcpy(this->_ps[n][1]+2,p-2,size+2);
                                    p+=size;
                                    p++;
                                    size = U16(p);
                                    p+=2;
                                    this->_ps[n][0] = new unsigned char[size+4];
                                    this->_ps[n][0][0] = 0;
                                    this->_ps[n][0][1] = 0;
                                    memcpy(this->_ps[n][0]+2,p-2,size+2);
#ifndef EMSCRIPTEN 
                                    
                                    this->_PS[n][0] =  [[NSData alloc] initWithBytes:this->_ps[n][0] length:(size+4)];
                                    this->_PS[n][1] =  [[NSData alloc] initWithBytes:this->_ps[n][1] length:(size+4)];
#endif
                                }
                            }
                        }
                            
                        double fps = 0;
                        for(int k=begin; k<end-(4*4); k++) {
                            if(U32(moov+k)==atom("stts")) {
                                fps = TimeScale/(double)(U32(moov+k+(4*4)));
                                break;
                            }
                        }
                        
                        if(fps>0) {
                            
                            info->fps = fps;
                            this->_info.push_back(info);
                            
                            unsigned int totalFrames = 0;
                            std::pair<u64,unsigned int> *frames = nullptr;
                            
                            for(int k=begin; k<end-((4*3)+4); k++) {
                                if(U32(moov+k)==atom("stsz")) {
                                    k+=(4*3);
                                    totalFrames = U32(moov+k);
                                    if(frames) delete[] frames;
                                    frames = new std::pair<u64,unsigned int>[totalFrames];
                                    for(int f=0; f<totalFrames; f++) {
                                        k+=4; // 32
                                        unsigned int size = U32(moov+k);
                                        frames[f] = std::make_pair(0,size);
                                    }
                                    break;
                                }
                            }
                                                            
                            for(int k=begin; k<end-((4*2)+4); k++) {
                                
                                if(U32(moov+k)==atom("stco")) {
                                    k+=(4*2);
                                    if(totalFrames==U32(moov+k)) {
                                        k+=4;
                                        for(int f=0; f<totalFrames; f++) {
                                            frames[f].first = U32(moov+k);
                                            k+=4; // 32
                                        }
                                        break;
                                    }
                                }
                                else if(U32(moov+k)==atom("co64")) {
                                    k+=(4*2);
                                    if(totalFrames==U32(moov+k)) {
                                        k+=4;
                                        for(int f=0; f<totalFrames; f++) {
                                            frames[f].first = U64(moov+k);
                                            k+=8; // 64
                                        }
                                        break;
                                    }
                                }
                            }
                            
                            this->_totalFrames.push_back(totalFrames);
                            this->_frames.push_back(frames);
                        }
                    }
                }
                
                if((this->_info.size()!=this->_tracks)||(this->_totalFrames.size()!=this->_tracks)||(this->_frames.size()!=this->_tracks)) {
                    
                    this->clear();
                }
                
            }
        
            unsigned char *vps(unsigned int track=0) { return this->_ps[track][2]; };
            unsigned char *sps(unsigned int track=0) { return this->_ps[track][1]; };
            unsigned char *pps(unsigned int track=0) { return this->_ps[track][0]; };

#ifndef EMSCRIPTEN 

            NSData *VPS(unsigned int track=0) { return this->_PS[track][2]; };
            NSData *SPS(unsigned int track=0) { return this->_PS[track][1]; };
            NSData *PPS(unsigned int track=0) { return this->_PS[track][0]; };
#endif

#ifdef EMSCRIPTEN 
        
            unsigned char *bytes() {
                return this->_bytes;
            };
        
            bool get(unsigned int current,unsigned int trackid, u64 *offset, unsigned int *size) {
                
                if(current<this->_totalFrames[trackid]) {
                    *offset = this->_frames[trackid][current].first;
                    *size = this->_frames[trackid][current].second;
                    return true;
                }
                
                return false;
            }
        
            void parse(unsigned char *bytes, u64 length) {
            
                this->reset();
                
                if(bytes&&length>0) {
                    
                    this->_bytes = bytes;
                    this->_length = length;
                    
                    unsigned char *seek = this->_bytes;
                    seek+=(4+4)+(U32(seek))+(4);
                    
                    if(U32(seek)==this->atom("mdat")) {
                                                
                        seek+=U32(seek-4);
                        
                        while(true) {
                            if(U32(seek)==this->atom("mdat")) {
                                seek+=U32(seek-4);
                            }
                            else if(U32(seek)==this->atom("moov")) {
                                unsigned char *moov = seek+4;
                                this->parseTrack(moov,U32(moov-8)-8);
                                break;
                            }
                            else {
                                break;
                            }
                        }
                    }
                    else if(U32(seek)==this->atom("mdat")) {
                        unsigned char *moov = seek+4;
                        this->parseTrack(moov,U32(moov-8)-8);
                        break;
                    }
                }

            }
        
            Parser(unsigned char *bytes, u64 length) {
                this->parse(bytes,length);
            }
            
            ~Parser() {
                this->_length = 0;
                this->_bytes = nullptr;
                this->clear();
            }
            
#else
        
            NSData *get(u64 n, unsigned int track) {
                if(n<this->_totalFrames[track]) {
                    [this->_handle seekToOffset:this->_frames[track][n].first error:nil];
                    return [this->_handle readDataUpToLength:this->_frames[track][n].second error:nil];
                }
                return nil;
            }
            
            Parser(NSString *path, std::vector<std::string> traks = {}) {
                
                this->_traks.push_back(this->atom("trak"));
                for(int n=0; n<traks.size(); n++) {
                    this->_traks.push_back(this->atom(traks[n]));
                }
                    
                if([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                    
                    this->_handle = [NSFileHandle fileHandleForReadingAtPath:path];
                                
                    unsigned long offset = 0;
                    
                    [this->_handle seekToOffset:0 error:nil];
                    NSData *data = [this->_handle readDataUpToLength:4 error:nil];
                    offset+=U32((unsigned char *)(data.bytes));
                    
                    [this->_handle seekToOffset:offset error:nil];
                    data = [this->_handle readDataUpToLength:4 error:nil];
                    offset+=U32((unsigned char *)(data.bytes));
                    offset+=4;
                    
                    [this->_handle seekToOffset:offset error:nil];
                    data = [this->_handle readDataUpToLength:4 error:nil];
                    
                    unsigned long mdat = 0;
                    
                    if(U32((unsigned char *)(data.bytes))==0x6D646174) { // mdat
                        
                        [this->_handle seekToOffset:offset-4 error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        if(U32((unsigned char *)(data.bytes))==1) {
                            [this->_handle seekToOffset:offset+4 error:nil];
                            data = [this->_handle readDataUpToLength:8 error:nil];
                            offset+=U64((unsigned char *)(data.bytes));
                        }
                        else {
                            offset+=U32((unsigned char *)(data.bytes));
                        }
                        
                        [this->_handle seekToOffset:offset error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        
                        if(U32((unsigned char *)(data.bytes))==0x6D6F6F76) { // moov
                            
                            [this->_handle seekToOffset:offset-4 error:nil];
                            data = [this->_handle readDataUpToLength:4 error:nil];
                            unsigned long length = U32((unsigned char *)(data.bytes));
                            
                            [this->_handle seekToOffset:offset+4 error:nil];
                            this->parseTrack((unsigned char *)[this->_handle readDataUpToLength:length-8 error:nil].bytes,length-8);
                            
                        }
                    }
                    else if(U32((unsigned char *)(data.bytes))==0x6D6F6F76) { // moov
                        
                        [this->_handle seekToOffset:offset-4 error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        unsigned long length = U32((unsigned char *)(data.bytes));
                        
                        [this->_handle seekToOffset:offset+4 error:nil];
                        this->parseTrack((unsigned char *)[this->_handle readDataUpToLength:length-8 error:nil].bytes,length-8);
                    }
                }
            }
        
            ~Parser() {
                if(this->_handle) [this->_handle closeFile];
            }
        
#endif
            
    };

};

                                
                                
                                