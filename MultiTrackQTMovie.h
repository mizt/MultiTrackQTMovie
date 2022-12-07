#import <vector>
#import <string>

#define USE_VECTOR

#ifdef EMSCRIPTEN
    typedef unsigned long long U64;
    typedef long long S64;
#else
    typedef unsigned long U64;
    typedef long S64;
#endif

#import "MultiTrackQTMovieParser.h"

namespace MultiTrackQTMovie {
    
    typedef std::pair<std::string,U64> Atom;

    // https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html
    class Moov {
        
        private:

#ifdef USE_VECTOR
            std::vector<unsigned char> bin;
            unsigned int CreationTime = 3061152000;
#else
            NSMutableData *bin = nil;
            unsigned int CreationTime = CFAbsoluteTimeGetCurrent() + kCFAbsoluteTimeIntervalSince1904;
#endif
        
            const bool is64 = true;
            const int Transfer = 1;
            unsigned int ModificationTime = CreationTime;
            unsigned int TimeScale = 30000;
            unsigned short Language = 0;
            
            void reset() {
#ifdef USE_VECTOR
                this->bin.clear();
#else
                if(this->bin) this->bin = nil; 
                this->bin = [[NSMutableData alloc] init];
#endif
            }
        
            void setU64(U64 value) {
#ifdef USE_VECTOR
                bin.push_back((value>>56)&0xFF);
                bin.push_back((value>>48)&0xFF);
                bin.push_back((value>>40)&0xFF);
                bin.push_back((value>>32)&0xFF);
                bin.push_back((value>>24)&0xFF);
                bin.push_back((value>>16)&0xFF);
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
#else
                [this->bin appendBytes:new unsigned long[1]{swapU64(value)} length:8];
#endif
            }
            
            void setU32(unsigned int value) {
#ifdef USE_VECTOR
                bin.push_back((value>>24)&0xFF);
                bin.push_back((value>>16)&0xFF);
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
#else
                [this->bin appendBytes:new unsigned int[1]{swapU32(value)} length:4];
#endif
            }
            
            void setU16(unsigned short value) {
#ifdef USE_VECTOR
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
#else
                [this->bin appendBytes:new unsigned short[1]{swapU16(value)} length:2];
#endif
            }
            
            void setU8(unsigned char value) {
#ifdef USE_VECTOR
                bin.push_back(value);
#else
                [bin appendBytes:new unsigned char[1]{value} length:1];
#endif
            }
            
            Atom initAtom(std::string str, unsigned int size=0, bool dump=false) {
                assert(str.length()<=4);
#ifdef USE_VECTOR
                U64 pos = this->bin.size();
#else
                U64 pos = (unsigned int)[bin length];
#endif
                this->setU32(size);
                //[this->bin appendBytes:new unsigned int[1]{swapU32(size)} length:4];
                this->setString(str);
                if(dump) NSLog(@"%s,%lu",str.c_str(),pos);
                return std::make_pair(str,pos);
            }
            
            void setAtomSize(U64 pos) {
#ifdef USE_VECTOR
                unsigned int size = (unsigned int)(this->bin.size()-pos);
                this->bin[pos+0] = (size>>24)&0xFF;
                this->bin[pos+1] = (size>>16)&0xFF;
                this->bin[pos+2] = (size>>8)&0xFF;
                this->bin[pos+3] = size&0xFF;
#else
                *(unsigned int *)(((unsigned char *)[this->bin bytes])+pos) = swapU32(((unsigned int)this->bin.length)-pos);
#endif
            }
        
            void setString(std::string str, unsigned int length=4) {
#ifdef USE_VECTOR
                unsigned char *s = (unsigned char *)str.c_str();
                for(S64 n=0; n<str.length(); n++) bin.push_back(s[n]);
                S64 len = length-str.length();
                if(len>=1) {
                    while(len--) bin.push_back(0);
                }
#else
                [this->bin appendBytes:(unsigned char *)str.c_str() length:str.length()];
                int len = length-(int)str.length();
                if(len>=1) {
                    while(len--) {
                        [this->bin appendBytes:new unsigned char[1]{0} length:1];
                    }
                }
#endif
            }
            
            void setPascalString(std::string str) {
                assert(str.length()<255);
#ifdef USE_VECTOR
                bin.push_back(str.size());
                this->setString(str,(unsigned int)str.size());
#else
                [this->bin appendBytes:new unsigned char[1]{(unsigned char)str.size()} length:1];
                this->setString(str,(unsigned int)str.size());
#endif
            }
            
            void setCompressorName(std::string str) {
                assert(str.length()<32);
#ifdef USE_VECTOR
                this->setPascalString(str);
                int len = 31-(int)str.length();
                while(len--) bin.push_back(0);
#else
                setPascalString(str);
                int len = 31-(int)str.length();
                while(len--) {
                    [this->bin appendBytes:new unsigned char[1]{0} length:1];
                }
#endif
            }
            
            void setMatrix() {
                // All values in the matrix are 32-bit fixed-point numbers divided as 16.16, except for the {u, v, w} column, which contains 32-bit fixed-point numbers divided as 2.30.
#ifdef USE_VECTOR
                this->setU32(0x00010000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00010000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x01000000);
                
#else
                [this->bin appendBytes:new unsigned int[4*9]{
                    swapU32(0x00010000),swapU32(0x00000000),swapU32(0x00000000),
                    swapU32(0x00000000),swapU32(0x00010000),swapU32(0x00000000),
                    swapU32(0x00000000),swapU32(0x00000000),swapU32(0x01000000)
                } length:(4*9)];
#endif
            }
            
            void setVersionWithFlag(unsigned char version=0, unsigned int flag=0) {
#ifdef USE_VECTOR
                bin.push_back(version);
                bin.push_back((flag>>16)&0xFF);
                bin.push_back((flag>>8)&0xFF);
                bin.push_back(flag&0xFF);
#else
                
                [this->bin appendBytes:new unsigned char[4]{
                    version,
                    (unsigned char)((flag>>16)&0xFF),
                    (unsigned char)((flag>>8)&0xFF),
                    (unsigned char)(flag&0xFF)} length:(1+3)];
#endif
            }
        
            void setZero(U64 length) {
#ifdef USE_VECTOR
                for(U64 n=0; n<length; n++) bin.push_back(0);
#else
                for(U64 n=0; n<length; n++) {
                    [this->bin appendBytes:new unsigned char[1]{0} length:1];
                }
#endif
            }
            
        public:
 
#ifdef USE_VECTOR
            std::vector<unsigned char> *get() {
                return &this->bin;
            }
#else
            NSMutableData *get() {
                return this->bin;
            }
#endif
            
            Moov(std::vector<TrackInfo> *info, std::vector<U64> *frames, std::vector<U64> *chunks, std::vector<bool> *keyframes, NSData *sps, NSData *pps) {
                                
                this->reset();
                
                Atom moov = this->initAtom("moov");
                
                unsigned int maxDuration = 0;
                for(int n=0; n<info->size(); n++) {
                    unsigned int TotalFrames = (unsigned int)frames[n].size();
                    float FPS = (float)((*info)[n].fps);
                    unsigned int Duration = (unsigned int)(TotalFrames*(this->TimeScale/FPS));
                    if(Duration>maxDuration) maxDuration = Duration;
                }
                
                Atom mvhd = this->initAtom("mvhd");
                this->setVersionWithFlag();
                this->setU32(this->CreationTime);
                this->setU32(this->ModificationTime);
                this->setU32(this->TimeScale);
                this->setU32(maxDuration);
                this->setU32(1<<16); // Preferred rate
                this->setU16(0); // Preferred volume
                this->setZero(10); // Reserved
                this->setMatrix();
                this->setU32(0); // Preview time
                this->setU32(0); // Preview duration
                this->setU32(0); // Poster time
                this->setU32(0); // Selection time
                this->setU32(0); // Selection duration
                this->setU32(0); // Current time
                this->setU32(((unsigned int)info->size())+1); // Next track ID
                this->setAtomSize(mvhd.second);
                
                for(int n=0; n<info->size(); n++) {
                    
                    bool avc1 = (((*info)[n].type)=="avc1")?true:false;
                    unsigned int track = n+1;

                    unsigned int TotalFrames = (unsigned int)frames[n].size();
                    float FPS = (float)((*info)[n].fps);
                    unsigned int SampleDelta = (unsigned int)(this->TimeScale/FPS);
                    unsigned int Duration = TotalFrames*SampleDelta;
                    
                    Atom trak = this->initAtom("trak");
                    Atom tkhd = this->initAtom("tkhd");
                    this->setVersionWithFlag(0,0xF);
                    this->setU32(CreationTime);
                    this->setU32(ModificationTime);
                    this->setU32(track); // Track id
                    this->setZero(4); // Reserved
                    this->setU32(Duration);
                    this->setZero(8); // Reserved
                    this->setU16(0); // Layer
                    this->setU16(0); // Alternate group
                    this->setU16(0); // Volume
                    this->setU16(0); // Reserved
                    this->setMatrix();
                    this->setU32(((*info)[n].width)<<16);
                    this->setU32(((*info)[n].height)<<16);
                    this->setAtomSize(tkhd.second);
                    Atom tapt = this->initAtom("tapt");
                    this->initAtom("clef",20);
                    this->setVersionWithFlag();
                    this->setU32(((*info)[n].width)<<16);
                    this->setU32(((*info)[n].height)<<16);
                    this->initAtom("prof",20);
                    this->setVersionWithFlag();
                    this->setU32(((*info)[n].width)<<16);
                    this->setU32(((*info)[n].height)<<16);
                    this->initAtom("enof",20);
                    this->setVersionWithFlag();
                    this->setU32(((*info)[n].width)<<16);
                    this->setU32(((*info)[n].height)<<16);
                    this->setAtomSize(tapt.second);
                    Atom edts = this->initAtom("edts");
                    Atom elst = this->initAtom("elst");
                    this->setVersionWithFlag();
                    this->setU32(1); // Number of entries
                    this->setU32(Duration);
                    this->setU32(0); // Media time
                    this->setU32(1<<16); // Media rate
                    this->setAtomSize(elst.second);
                    this->setAtomSize(edts.second);
                    Atom mdia = this->initAtom("mdia");
                    Atom mdhd = this->initAtom("mdhd");
                    this->setVersionWithFlag();
                    this->setU32(CreationTime);
                    this->setU32(ModificationTime);
                    this->setU32(TimeScale);
                    this->setU32(Duration);
                    this->setU16(Language);
                    this->setU16(0); // Quality
                    this->setAtomSize(mdhd.second);
                    Atom hdlr = this->initAtom("hdlr");
                    this->setVersionWithFlag();
                    this->setString("mhlr");
                    this->setString("vide");
                    this->setU32(0); // Reserved
                    this->setZero(8); // Reserved
                    this->setPascalString("Video");
                    this->setAtomSize(hdlr.second);
                    Atom minf = this->initAtom("minf");
                    Atom vmhd = this->initAtom("vmhd");
                    this->setVersionWithFlag(0,1);
                    this->setU16(0); // Graphics mode 64? Copy = 0, 64 = Dither Copy
                    this->setU16(32768); // Opcolor
                    this->setU16(32768); // Opcolor
                    this->setU16(32768); // Opcolor
                    this->setAtomSize(vmhd.second);
                    hdlr = this->initAtom("hdlr");
                    this->setVersionWithFlag();
                    this->setString("dhlr");
                    this->setString("alis");
                    this->setU32(0); // Reserved 0
                    this->setZero(8); // Reserved
                    this->setPascalString("Handler");
                    this->setAtomSize(hdlr.second);
                    Atom dinf = this->initAtom("dinf");
                    Atom dref = this->initAtom("dref");
                    this->setVersionWithFlag();
                    this->setU32(1); // Number of entries
                    this->initAtom("alis",12);
                    this->setVersionWithFlag(0,1);
                    this->setAtomSize(dref.second);
                    this->setAtomSize(dinf.second);
                    Atom stbl = this->initAtom("stbl");
                    Atom stsd = this->initAtom("stsd");
                    this->setVersionWithFlag();
                    this->setU32(1); // Number of entries
                    
                    Atom table = initAtom(((*info)[n].type));
                    this->setZero(6); // Reserved
                    this->setU16(1); // Data reference index
                    this->setU16(0); // Version
                    this->setU16(0); // Revision level
                    this->setU32(0); // Vendor
                    this->setU32(0); // Temporal quality
                    this->setU32(1024); // Spatial quality
                    this->setU16(((*info)[n].width));
                    this->setU16(((*info)[n].height));
                    this->setU32(72<<16); // Horizontal resolution
                    this->setU32(72<<16); // Vertical resolution
                    this->setZero(4);
                    this->setU16(1); // Frame count
                    
                    if(!avc1) {
                        this->setCompressorName("'"+((*info)[n].type)+"'"); // 32
                    }
                    else {
                        this->setCompressorName("H.264"); // 32
                    }
                    
                    this->setU16(((*info)[n].depth)); // Depth
                    this->setU16(0xFFFF); // Color table ID
                    
                    if(avc1&&sps&&pps) {
                        
                        Atom avcC = this->initAtom("avcC");
                        this->setU8(1);
                        this->setU8(66);
                        this->setU8(0);
                        this->setU8(40);
                        this->setU8(0xFF); // 3
                        this->setU8(0xE1); // 1
                        
#ifdef USE_VECTOR
                        this->setU16(swapU32(*((unsigned int *)[sps bytes]))&0xFFFF);
                        
                        unsigned char *bytes = ((unsigned char *)[sps bytes])+4;
                        for(U64 n=0; n<[sps length]-4; n++) {
                            this->bin.push_back(bytes[n]);
                        }
                        
                        this->setU8(1); // 1
                        this->setU16(swapU32(*((unsigned int *)[pps bytes]))&0xFFFF);
                        
                        bytes = ((unsigned char *)[pps bytes])+4;
                        for(U64 n=0; n<[pps length]-4; n++) {
                            this->bin.push_back(bytes[n]);
                        }
                        
#else
                        this->setU16(swapU32(*((unsigned int *)[sps bytes]))&0xFFFF);
                        [this->bin appendBytes:((unsigned char *)[sps bytes])+4 length:[sps length]-4];
                        
                        this->setU8(1); // 1
                        this->setU16(swapU32(*((unsigned int *)[pps bytes]))&0xFFFF);
                        [this->bin appendBytes:((unsigned char *)[pps bytes])+4 length:[pps length]-4];
#endif
                        
                        this->setAtomSize(avcC.second);
                    }
                    
                    this->initAtom("colr",18);
                    this->setString("nclc");
                    this->setU16(1); // Primaries index
                    this->setU16(Transfer); // Transfer function index
                    this->setU16(1); // Matrix index
                    if(this->Transfer==2) {
                        this->initAtom("gama",12);
                        this->setU32(144179); // 2.2 = 144179, 1.96 = 128512
                    }
                    
                    if(!avc1) {
                        this->initAtom("fiel",10);
                        this->setU16(1<<8);
                        this->initAtom("pasp",16);
                        this->setU32(1);
                        this->setU32(1);
                    }
                    
                    this->setU32(0); // Some sample descriptions terminate with four zero bytes that are not otherwise indicated.
                    this->setAtomSize(table.second);
                    this->setAtomSize(stsd.second);
                    
                    this->initAtom("stts",24);
                    this->setVersionWithFlag();
                    
                    this->setU32(1); // Number of entries
                    this->setU32(TotalFrames);
                    this->setU32(SampleDelta);
                    
                    if(avc1) {
                        
                        this->initAtom("stss",8+4+4+TotalFrames*4);
                        this->setVersionWithFlag();
                        this->setU32(TotalFrames);
                        for(int k=0; k<TotalFrames; k++) {
                            this->setU32(k+1); // 1 origin
                        }
                        
                        this->initAtom("sdtp",8+4+TotalFrames);
                        this->setVersionWithFlag();
                        
                        for(int k=0; k<TotalFrames; k++) {
                            this->setU8((keyframes[n][k])?32:16);
                        }
                    }
                    
                    this->initAtom("stsc",28);
                    this->setVersionWithFlag();
                    this->setU32(1); // Number of entries
                    this->setU32(1); // First chunk
                    this->setU32(1); // Samples per chunk
                    this->setU32(1); // Sample description ID
                    Atom stsz = this->initAtom("stsz",20);
                    this->setVersionWithFlag();
                    this->setU32(0); // If this field is set to 0, then the samples have different sizes, and those sizes are stored in the sample size table.
                    this->setU32(TotalFrames); // Number of entries
                    for(int k=0; k<TotalFrames; k++) {
                        this->setU32((unsigned int)frames[n][k]);
                    }
                    
                    if(this->is64) {
                        this->setAtomSize(stsz.second);
                        Atom co64 = this->initAtom("co64");
                        this->setVersionWithFlag();
                        this->setU32((unsigned int)chunks[n].size()); // Number of entries
                        for(int k=0; k<chunks[n].size(); k++) {
                            this->setU64(chunks[n][k]); // Chunk
                        }
                        this->setAtomSize(co64.second);
                    }
                    else {
                        this->setAtomSize(stsz.second);
                        Atom stco = this->initAtom("stco");
                        this->setVersionWithFlag();
                        this->setU32((unsigned int)chunks[n].size()); // Number of entries
                        for(int k=0; k<chunks[n].size(); k++) {
                            this->setU32((unsigned int)chunks[n][k]); // Chunk
                        }
                        this->setAtomSize(stco.second);
                    }
                    
                    this->setAtomSize(minf.second);
                    this->setAtomSize(stbl.second);
                    this->setAtomSize(mdia.second);
                    this->setAtomSize(trak.second);
                }
                
                this->setAtomSize(moov.second);
                
            }
            
            ~Moov() {
                this->reset();
            }
    };
    
    
    class VideoRecorder {
        
        protected:
            
            
            NSString *_fileName;
            
            bool _isRunning = false;
            bool _isRecorded = false;
            
            NSFileHandle *_handle;
            std::vector<U64> *_frames;
            
            const unsigned int MDAT_LIMIT = (1024*1024*1024)/10; // 0.1 = 100MB
            U64 _mdat_offset = 0;
            NSMutableData *_mdat = nil;
            
            U64 _chunk_offset = 0;
            std::vector<U64> *_chunks;
        
            std::vector<bool> *_keyframes;
            
            std::vector<TrackInfo> *_info;
            
            NSData *_sps = nil;
            NSData *_pps = nil;
        
            NSString *filename() {
                long unixtime = (CFAbsoluteTimeGetCurrent()+kCFAbsoluteTimeIntervalSince1970)*1000;
                NSString *timeStampString = [NSString stringWithFormat:@"%f",(unixtime/1000.0)];
                NSDate *date = [NSDate dateWithTimeIntervalSince1970:[timeStampString doubleValue]];
                NSDateFormatter *format = [[NSDateFormatter alloc] init];
                [format setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"ja_JP"]];
                [format setDateFormat:@"yyyy_MM_dd_HH_mm_ss_SSS"];
    #if TARGET_OS_OSX
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSMoviesDirectory,NSUserDomainMask,YES);
    #else
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES);
    #endif
                NSString *directory = [paths objectAtIndex:0];
                return [NSString stringWithFormat:@"%@/%@",directory,[format stringFromDate:date]];
            }
            
            
        public:
            
            VideoRecorder(NSString *fileName, std::vector<TrackInfo> *info, NSData *sps=nil, NSData *pps=nil) {
                
                if(fileName) this->_fileName = fileName;
                else this->_fileName = [NSString stringWithFormat:@"%@.mov",this->filename()];
                this->_info = info;
                this->_frames = new std::vector<U64>[this->_info->size()];
                this->_chunks = new std::vector<U64>[this->_info->size()];
                this->_keyframes = new std::vector<bool>[this->_info->size()];
                 
                if(sps) this->_sps = sps;
                if(pps) this->_pps = pps;

            }
            ~VideoRecorder() {
                for(int k=0; k<this->_info->size(); k++) {
                    this->_frames[k].clear();
                    this->_chunks[k].clear();
                    this->_keyframes[k].clear();
                }
                delete[] this->_frames;
                delete[] this->_chunks;
                delete[] this->_keyframes;

            }
    };
    
    class Recorder : public VideoRecorder {
        
        protected:
            
            
            
            void inialized() {
                
                NSMutableData *bin = [[NSMutableData alloc] init];
                /*
                this->initAtom("ftyp",20);
                this->setString(bin,"qt  ",4);
                this->setU32(0);
                this->setString(bin,"qt  ",4);
                this->initAtom("wide",8);
                */
                
                [bin appendBytes:new unsigned int[7]{
                    swapU32(0x00000014), // 20
                    swapU32(0x66747970), // 'ftyp'
                    swapU32(0x71742020), // 'qt  '
                    swapU32(0x00000000),
                    swapU32(0x71742020), // 'qt  '
                    swapU32(0x00000008), // 8
                    swapU32(0x77696465)  // 'wide'
                } length:7*4];

                [bin writeToFile:this->_fileName options:NSDataWritingAtomic error:nil];
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_fileName];
                [this->_handle seekToEndOfFile];
                
                this->_mdat_offset = [bin length];
                this->_chunk_offset = [bin length];
            }
            
        public:
            
            Recorder(NSString *fileName,std::vector<TrackInfo> *info, NSData *sps = nil, NSData *pps = nil) : VideoRecorder(fileName,info,sps,pps) {
                this->inialized();
            }
            
            Recorder(std::vector<TrackInfo> *info, NSData *sps = nil, NSData *pps = nil) : VideoRecorder(nil,info,sps,pps) {
                this->inialized();
            }
            
            Recorder *add(unsigned char *data, unsigned long length, unsigned int trackid, bool keyframe=true, bool padding=true) {
                
                if(!this->_isRecorded) {
                    
                    if(this->_isRunning==false) this->_isRunning = true;
                    
                    if(trackid>=0&&trackid<this->_info->size()) {
                        
                        U64 size = length;
                        U64 diff = 0;
                        
                        if(padding&&(length%4!=0)) {
                            diff=(((length+3)>>2)<<2)-length;
                            size+=diff;
                        }
                        
                        if(this->_mdat&&([this->_mdat length]+size)>=MDAT_LIMIT) {
                            
                            [this->_handle seekToEndOfFile];
                            [this->_handle writeData:this->_mdat];
                            
                            [this->_handle seekToFileOffset:this->_mdat_offset];
                            NSData *tmp = [[NSData alloc] initWithBytes:new unsigned int[1]{swapU32((unsigned int)[this->_mdat length])} length:4];
                            [this->_handle writeData:tmp];
                            [this->_handle seekToEndOfFile];
                            
                            this->_mdat_offset+=[this->_mdat length];
                            this->_mdat = nil;
                        }
                        
                        if(this->_mdat==nil) {
                            
                            this->_mdat = [[NSMutableData alloc] init];
                            [this->_mdat appendBytes:new unsigned int[1]{swapU32(0)} length:4];
                            [this->_mdat appendBytes:new unsigned int[1]{
                                swapU32(0x6D646174) // 'mdat'
                            } length:4];
                            this->_chunk_offset+=8;
                        }
                        
                        if(((*this->_info)[trackid].type)=="avc1") {
                            this->_frames[trackid].push_back(length);
                        }
                        else {
                            this->_frames[trackid].push_back(size);
                        }
                        
                        this->_chunks[trackid].push_back(this->_chunk_offset);
                        
                        this->_keyframes[trackid].push_back(keyframe);

                        
                        [this->_mdat appendBytes:data length:length];
                        if(diff) {
                            [this->_mdat appendBytes:new unsigned char[diff]{0} length:diff];
                        }
                        
                        this->_chunk_offset+=size;
                    }
                }
                
                return this;
            }
            
            void save() {
                
                if(this->_isRunning&&!this->_isRecorded) {
                    
                    for(int n=0; n<this->_info->size(); n++) {
                        bool avc1 = (((*this->_info)[n].type)=="avc1")?true:false;
                        if(avc1) {
                            if(!(this->_sps&&this->_pps)) {
                                this->_isRunning = false;
                                this->_isRecorded = true;
                                return;
                            }
                        }
                    }
                    
                    if(this->_mdat) {
                        [this->_handle seekToEndOfFile];
                        [this->_handle writeData:this->_mdat];
                        [this->_handle seekToFileOffset:this->_mdat_offset];
                        NSData *tmp = [[NSData alloc] initWithBytes:new unsigned int[1]{0} length:4];
                        *((unsigned int *)[tmp bytes]) = swapU32((unsigned int)[this->_mdat length]);
                        [this->_handle writeData:tmp];
                        [this->_handle seekToEndOfFile];
                        this->_mdat = nil;
                    }
                                        
                    Moov *moov = new Moov(this->_info,this->_frames,this->_chunks,this->_keyframes,this->_sps,this->_pps);
                    
                    [this->_handle seekToEndOfFile];
                    
#ifdef USE_VECTOR
                    std::vector<unsigned char> *bin = moov->get();
                    [this->_handle writeData:[[NSData alloc] initWithBytes:bin->data() length:bin->size()]];

#else
                    [this->_handle writeData:moov->get()];

#endif
                    delete moov;
                    
                    this->_isRunning = false;
                    this->_isRecorded = true;
                }
            }
            
            ~Recorder() {
                
            }
    };

};

