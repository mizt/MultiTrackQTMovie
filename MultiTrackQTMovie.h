#import <vector>
#import <string>

#import "MultiTrackQTMovieParser.h"

namespace MultiTrackQTMovie {
    
    class VideoRecorder {
        
        protected:
            
            NSString *_fileName;
            
            bool _isRunning = false;
            bool _isRecorded = false;
            
            NSFileHandle *_handle;
            std::vector<unsigned int> *_frames;
            
            const unsigned int MDAT_LIMIT = (1024*1024*1024)/10; // 0.1 = 100MB
            unsigned long _mdat_offset = 0;
            NSMutableData *_mdat = nil;
            
            unsigned long _chunk_offset = 0;
            std::vector<unsigned long> *_chunks;
            
            std::vector<TrackInfo> *_info;
            
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
            
            void setString(NSMutableData *bin,std::string str,unsigned int length=4) {
                [bin appendBytes:(unsigned char *)str.c_str() length:str.length()];
                int len = length-(int)str.length();
                if(len>=1) {
                    while(len--) {
                        [bin appendBytes:new unsigned char[1]{0} length:1];
                    }
                }
            }
            
        public:
            
            VideoRecorder(NSString *fileName,std::vector<TrackInfo> *info,unsigned short FPS=30) {
                if(fileName) this->_fileName = fileName;
                else this->_fileName = [NSString stringWithFormat:@"%@.mov",this->filename()];
                this->_info = info;
                this->_frames = new std::vector<unsigned int>[this->_info->size()];
                this->_chunks = new std::vector<unsigned long>[this->_info->size()];
            }
            virtual void add(unsigned int *data,int length) {}
            virtual void save() {}
            ~VideoRecorder() {
                for(int k=0; k<this->_info->size(); k++) {
                    this->_frames[k].clear();
                    this->_chunks[k].clear();
                }
                delete[] this->_frames;
                delete[] this->_chunks;
            }
    };
    
    // https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html
    class Recorder : public VideoRecorder {
        
        private:
            
            const int Transfer = 2;
            typedef std::pair<std::string,unsigned int> Atom;
            
        protected:
            
            unsigned int CreationTime = CFAbsoluteTimeGetCurrent() + kCFAbsoluteTimeIntervalSince1904;
            unsigned int ModificationTime = CreationTime;
            unsigned int TimeScale = 30000;
            unsigned short Language = 0;
            
            void setU64(NSMutableData *bin,unsigned long value) {
                [bin appendBytes:new unsigned long[1]{swapU64(value)} length:8];
            }
            
            void setU32(NSMutableData *bin,unsigned int value) {
                [bin appendBytes:new unsigned int[1]{swapU32(value)} length:4];
            }
            
            void setU16(NSMutableData *bin,unsigned short value) {
                [bin appendBytes:new unsigned short[1]{swapU16(value)} length:2];
            }
            
            Atom initAtom(NSMutableData *bin,std::string str, unsigned int size=0,bool dump=false) {
                assert(str.length()<=4);
                unsigned int pos = (unsigned int)[bin length];
                [bin appendBytes:new unsigned int[1]{swapU32(size)} length:4];
                setString(bin,str);
                if(dump) NSLog(@"%s,%d",str.c_str(),pos);
                return std::make_pair(str,pos);
            }
            
            void setAtomSize(NSMutableData *bin,unsigned int pos) {
                *(unsigned int *)(((unsigned char *)[bin bytes])+pos) = swapU32(((unsigned int)bin.length)-pos);
            }
            
            void setPascalString(NSMutableData *bin,std::string str) {
                assert(str.length()<255);
                [bin appendBytes:new unsigned char[1]{(unsigned char)str.size()} length:1];
                setString(bin,str,(unsigned int)str.size());
            }
            
            void setCompressorName(NSMutableData *bin,std::string str) {
                assert(str.length()<32);
                setPascalString(bin,str);
                int len = 31-(int)str.length();
                while(len--) {
                    [bin appendBytes:new unsigned char[1]{0} length:1];
                }
            }
            
            void setMatrix(NSMutableData *bin) {
                // All values in the matrix are 32-bit fixed-point numbers divided as 16.16, except for the {u, v, w} column, which contains 32-bit fixed-point numbers divided as 2.30.
                [bin appendBytes:new unsigned int[4*9]{
                    swapU32(0x00010000),swapU32(0x00000000),swapU32(0x00000000),
                    swapU32(0x00000000),swapU32(0x00010000),swapU32(0x00000000),
                    swapU32(0x00000000),swapU32(0x00000000),swapU32(0x01000000)
                } length:(4*9)];
            }
            
            void setVersionWithFlag(NSMutableData *bin,unsigned char version=0,unsigned int flag=0) {
                [bin appendBytes:new unsigned char[4]{
                    version,
                    (unsigned char)((flag>>16)&0xFF),
                    (unsigned char)((flag>>8)&0xFF),
                    (unsigned char)(flag&0xFF)} length:(1+3)];
            }
            
            void inialized() {
                
                NSMutableData *bin = [[NSMutableData alloc] init];
                this->initAtom(bin,"ftyp",20);
                this->setString(bin,"qt  ",4);
                this->setU32(bin,0);
                this->setString(bin,"qt  ",4);
                this->initAtom(bin,"wide",8);
                
                [bin writeToFile:this->_fileName options:NSDataWritingAtomic error:nil];
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_fileName];
                [this->_handle seekToEndOfFile];
                
                this->_mdat_offset = [bin length];
                this->_chunk_offset = [bin length];
            }
            
        public:
            
            Recorder(NSString *fileName,std::vector<TrackInfo> *info) : VideoRecorder(fileName,info) {
                this->inialized();
            }
            
            Recorder(std::vector<TrackInfo> *info) : VideoRecorder(nil,info) {
                this->inialized();
            }
            
            Recorder *add(unsigned char *data,unsigned int length, unsigned int trackid) {
                if(!this->_isRecorded) {
                    if(this->_isRunning==false) this->_isRunning = true;
                    
                    if(trackid>=0&&trackid<this->_info->size()) {
                        
                        unsigned int size = length;
                        unsigned int diff = 0;
                        if(length%4!=0) {
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
                            setString(this->_mdat,"mdat");
                            this->_chunk_offset+=8;
                        }
                        
                        this->_frames[trackid].push_back(size);
                        this->_chunks[trackid].push_back(this->_chunk_offset);
                        
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
                    
                    NSMutableData *bin = [[NSMutableData alloc] init];
                    Atom moov = this->initAtom(bin,"moov");
                    
                    unsigned int maxDuration = 0;
                    for(int n=0; n<this->_info->size(); n++) {
                        unsigned int duration = (unsigned int)this->_frames[n].size()*(this->TimeScale/(float)((*this->_info)[n].fps));
                        if(duration>maxDuration) maxDuration = duration;
                    }
                    
                    Atom mvhd = this->initAtom(bin,"mvhd");
                    this->setVersionWithFlag(bin);
                    this->setU32(bin,this->CreationTime);
                    this->setU32(bin,this->ModificationTime); 
                    this->setU32(bin,this->TimeScale);
                    this->setU32(bin,maxDuration);
                    this->setU32(bin,1<<16); // Preferred rate
                    this->setU16(bin,0); // Preferred volume
                    [bin appendBytes:new unsigned char[10]{0} length:(10)]; // Reserved
                    this->setMatrix(bin);
                    this->setU32(bin,0); // Preview time
                    this->setU32(bin,0); // Preview duration
                    this->setU32(bin,0); // Poster time
                    this->setU32(bin,0); // Selection time
                    this->setU32(bin,0); // Selection duration
                    this->setU32(bin,0); // Current time
                    this->setU32(bin,((unsigned int)this->_info->size())+1); // Next track ID
                    this->setAtomSize(bin,mvhd.second);
                    
                    for(int n=0; n<this->_info->size(); n++) {
                        unsigned int track = n+1;
                        unsigned int Duration = (unsigned int)this->_frames[n].size()*(this->TimeScale/(float)((*this->_info)[n].fps));
                        
                        Atom trak = this->initAtom(bin,"trak");
                        Atom tkhd = this->initAtom(bin,"tkhd");
                        this->setVersionWithFlag(bin,0,0xF);
                        this->setU32(bin,CreationTime);
                        this->setU32(bin,ModificationTime);
                        this->setU32(bin,track); // Track id
                        [bin appendBytes:new unsigned int[1]{0} length:(4)]; // Reserved
                        this->setU32(bin,Duration);
                        [bin appendBytes:new unsigned int[2]{0} length:(8)]; // Reserved
                        this->setU16(bin,0); // Layer
                        this->setU16(bin,0); // Alternate group
                        this->setU16(bin,0); // Volume
                        this->setU16(bin,0); // Reserved
                        this->setMatrix(bin);
                        this->setU32(bin,((*this->_info)[n].width)<<16);
                        this->setU32(bin,((*this->_info)[n].height)<<16);
                        this->setAtomSize(bin,tkhd.second);
                        Atom tapt = this->initAtom(bin,"tapt");
                        this->initAtom(bin,"clef",20);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,((*this->_info)[n].width)<<16);
                        this->setU32(bin,((*this->_info)[n].height)<<16);
                        this->initAtom(bin,"prof",20);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,((*this->_info)[n].width)<<16);
                        this->setU32(bin,((*this->_info)[n].height)<<16);
                        this->initAtom(bin,"enof",20);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,((*this->_info)[n].width)<<16);
                        this->setU32(bin,((*this->_info)[n].height)<<16);
                        this->setAtomSize(bin,tapt.second);
                        Atom edts = initAtom(bin,"edts");
                        Atom elst = initAtom(bin,"elst");
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,1); // Number of entries
                        this->setU32(bin,Duration);
                        this->setU32(bin,0); // Media time
                        this->setU32(bin,1<<16); // Media rate
                        this->setAtomSize(bin,elst.second);
                        this->setAtomSize(bin,edts.second);
                        Atom mdia = this->initAtom(bin,"mdia");
                        Atom mdhd = this->initAtom(bin,"mdhd");
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,CreationTime);
                        this->setU32(bin,ModificationTime);
                        this->setU32(bin,TimeScale);
                        this->setU32(bin,Duration);
                        this->setU16(bin,Language);
                        this->setU16(bin,0); // Quality
                        this->setAtomSize(bin,mdhd.second);
                        Atom hdlr = initAtom(bin,"hdlr");
                        this->setVersionWithFlag(bin);
                        this->setString(bin,"mhlr");
                        this->setString(bin,"vide");
                        this->setU32(bin,0); // Reserved
                        [bin appendBytes:new unsigned int[2]{0,0} length:8]; // Reserved
                        this->setPascalString(bin,"Video");
                        this->setAtomSize(bin,hdlr.second);
                        Atom minf = initAtom(bin,"minf");
                        Atom vmhd = initAtom(bin,"vmhd");
                        this->setVersionWithFlag(bin,0,1);
                        this->setU16(bin,0); // Graphics mode
                        this->setU16(bin,32768); // Opcolor
                        this->setU16(bin,32768); // Opcolor
                        this->setU16(bin,32768); // Opcolor
                        this->setAtomSize(bin,vmhd.second);
                        hdlr = initAtom(bin,"hdlr");
                        this->setVersionWithFlag(bin);
                        this->setString(bin,"dhlr");
                        this->setString(bin,"alis");
                        this->setU32(bin,0); // Reserved 0
                        [bin appendBytes:new unsigned int[2]{0,0} length:8]; // Reserved
                        this->setPascalString(bin,"Handler");
                        this->setAtomSize(bin,hdlr.second);
                        Atom dinf = this->initAtom(bin,"dinf");
                        Atom dref = this->initAtom(bin,"dref");
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,1); // Number of entries
                        this->initAtom(bin,"alis",12);
                        this->setVersionWithFlag(bin,0,1);
                        this->setAtomSize(bin,dref.second);
                        this->setAtomSize(bin,dinf.second);
                        Atom stbl = this->initAtom(bin,"stbl");
                        Atom stsd = this->initAtom(bin,"stsd");
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,1); // Number of entries
                        Atom table = initAtom(bin,((*this->_info)[n].type));
                        [bin appendBytes:new unsigned char[6]{0,0,0,0,0,0} length:(6)]; // Reserved
                        this->setU16(bin,1); // Data reference index
                        this->setU16(bin,0); // Version
                        this->setU16(bin,0); // Revision level
                        this->setU32(bin,0); // Vendor
                        this->setU32(bin,0); // Temporal quality
                        this->setU32(bin,1024); // Spatial quality
                        this->setU16(bin,((*this->_info)[n].width));
                        this->setU16(bin,((*this->_info)[n].height));
                        this->setU32(bin,72<<16); // Horizontal resolution
                        this->setU32(bin,72<<16); // Vertical resolution
                        [bin appendBytes:new unsigned int[1]{0} length:4];
                        this->setU16(bin,1); // Frame count
                        this->setCompressorName(bin,"'"+((*this->_info)[n].type)+"'"); // 32
                        this->setU16(bin,((*this->_info)[n].depth)); // Depth
                        this->setU16(bin,0xFFFF); // Color table ID
                        this->initAtom(bin,"colr",18);
                        this->setString(bin,"nclc");
                        this->setU16(bin,1); // Primaries index
                        this->setU16(bin,Transfer); // Transfer function index
                        this->setU16(bin,1); // Matrix index
                        if(this->Transfer==2) {
                            this->initAtom(bin,"gama",12);
                            this->setU32(bin,144179); // 2.2 = 144179, 1.96 = 128512
                        }
                        this->initAtom(bin,"fiel",10);
                        this->setU16(bin,1<<8);
                        this->initAtom(bin,"pasp",16);
                        this->setU32(bin,1);
                        this->setU32(bin,1);
                        this->setU32(bin,0); // Some sample descriptions terminate with four zero bytes that are not otherwise indicated.
                        this->setAtomSize(bin,table.second);
                        this->setAtomSize(bin,stsd.second);
                        this->initAtom(bin,"stts",24);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,1); // Number of entries
                        this->setU32(bin,(unsigned int)this->_frames[n].size());
                        this->setU32(bin,(unsigned int)(TimeScale/(*this->_info)[n].fps));
                        this->initAtom(bin,"stsc",28);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,1); // Number of entries
                        this->setU32(bin,1); // First chunk
                        this->setU32(bin,1); // Samples per chunk
                        this->setU32(bin,1); // Sample description ID
                        Atom stsz = this->initAtom(bin,"stsz",20);
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,0); // If this field is set to 0, then the samples have different sizes, and those sizes are stored in the sample size table.
                        this->setU32(bin,(unsigned int)this->_frames[n].size()); // Number of entries
                        for(int k=0; k<this->_frames[n].size(); k++) {
                            this->setU32(bin,this->_frames[n][k]);
                        }
                        this->setAtomSize(bin,stsz.second);
                        Atom stco = initAtom(bin,"co64");
                        this->setVersionWithFlag(bin);
                        this->setU32(bin,(unsigned int)this->_chunks[n].size()); // Number of entries
                        for(int k=0; k<this->_chunks[n].size(); k++) {
                            this->setU64(bin,this->_chunks[n][k]); // Chunk
                        }
                        this->setAtomSize(bin,minf.second);
                        this->setAtomSize(bin,stco.second);
                        this->setAtomSize(bin,stbl.second);
                        this->setAtomSize(bin,mdia.second);
                        this->setAtomSize(bin,trak.second);
                    }
                    
                    this->setAtomSize(bin,moov.second);
                    [this->_handle seekToEndOfFile];
                    [this->_handle writeData:bin];
                    
                    this->_isRunning = false;
                    this->_isRecorded = true;
                }
            }
            
            ~Recorder() {
                
            }
    };

};

