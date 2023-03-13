#import <vector>
#import <string>

#import "MultiTrackQTMovieEvent.h"
#import "MultiTrackQTMovieUtils.h"

namespace MultiTrackQTMovie {
    class SampleData {
        
    private:
        
        unsigned int _tracks = 1;
        
        unsigned long _offset = 0;
        unsigned long _length = 0;
        unsigned int _chank = 1;
        
        std::vector<bool> *_keyframes;
        std::vector<unsigned long> *_offsets;
        std::vector<unsigned int> *_lengths;
        std::vector<unsigned int> *_chunks;
        
    public:
        
        std::vector<bool> *keyframes(unsigned int trackid) { return this->_keyframes+trackid; }
        std::vector<unsigned long> *offsets(unsigned int trackid) { return this->_offsets+trackid; }
        std::vector<unsigned int> *lengths(unsigned int trackid) { return this->_lengths+trackid; };
        std::vector<unsigned int> *chunks(unsigned int trackid) { return this->_chunks+trackid; };
        
        void writeData(NSFileHandle *handle, unsigned char *bytes, unsigned int length, bool keyframe, unsigned int trackid) {
            this->_offsets[trackid].push_back(this->_offset+this->_length);
            this->_lengths[trackid].push_back(length);
            this->_chunks[trackid].push_back(this->_chank++);
            [handle writeData:[[NSData alloc] initWithBytes:bytes length:length]];
            [handle seekToEndOfFile];
            this->_length+=length;
            this->_keyframes[trackid].push_back(keyframe);
        }
        
        void writeSize(NSFileHandle *handle) {
            [handle seekToFileOffset:this->_offset+8];
            [handle writeData:[[NSData alloc] initWithBytes:new unsigned char[8]{
                (unsigned char)((this->_length>>56)&0xFF),
                (unsigned char)((this->_length>>48)&0xFF),
                (unsigned char)((this->_length>>40)&0xFF),
                (unsigned char)((this->_length>>32)&0xFF),
                (unsigned char)((this->_length>>24)&0xFF),
                (unsigned char)((this->_length>>16)&0xFF),
                (unsigned char)((this->_length>>8)&0xFF),
                (unsigned char)(this->_length&0xFF)} length:8]];
            [handle seekToEndOfFile];
        }
        
        SampleData(NSFileHandle *handle, unsigned long offset,unsigned int tracks) {
            
            this->_tracks = tracks;
            
            this->_keyframes = new std::vector<bool>[this->_tracks];
            this->_offsets = new std::vector<unsigned long>[this->_tracks];
            this->_lengths = new std::vector<unsigned int>[this->_tracks];
            this->_chunks = new std::vector<unsigned int>[this->_tracks];
            
            this->_offset = offset;
            [handle writeData:[[NSData alloc] initWithBytes:new unsigned char[16]{
                0,0,0,1,
                'm','d','a','t',
                0,0,0,0,
                0,0,0,0
            } length:16]];
            [handle seekToEndOfFile];
            this->_length+=16;
        }
        
        ~SampleData() {
            for(int n=0; n<this->_tracks; n++) {
                this->_keyframes[n].clear();
                this->_offsets[n].clear();
                this->_lengths[n].clear();
                this->_chunks[n].clear();
            }
        }
    };
}

// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html

namespace MultiTrackQTMovie {
    
    class ftyp : public AtomUtils {
        
    public:
        
        std::vector<unsigned char> *get() {
            return &this->bin;
        }
        
        ftyp() {
            this->reset();
            this->setU32(0x00000014); // 20
            this->setU32(0x66747970); // 'ftyp'
            this->setU32(0x71742020); // 'qt  '
            this->setU32(0x00000000);
            this->setU32(0x71742020); // 'qt  '
            this->setU32(0x00000008); // 8
            this->setU32(0x77696465); // 'wide'
        }
        
        ~ftyp() {
            this->reset();
        }
    };
}

namespace MultiTrackQTMovie {
    
    class moov : public AtomUtils {
        
    private:
        
        const bool is64 = false;
        const int Transfer = 1;
        unsigned int ModificationTime = CreationTime;
        unsigned int TimeScale = 30000;
        unsigned short Language = 21956;
        
        Atom initAtom(std::string str, unsigned int size=0, bool dump=false) {
            //assert(str.length()<=4);
            u64 pos = this->bin.size();
            this->setU32(size);
            this->setString(str);
            //if(dump) NSLog(@"%s,%lu",str.c_str(),pos);
            return std::make_pair(str,pos);
        }
        
        void setAtomSize(u64 pos) {
            unsigned int size = (unsigned int)(this->bin.size()-pos);
            this->bin[pos+0] = (size>>24)&0xFF;
            this->bin[pos+1] = (size>>16)&0xFF;
            this->bin[pos+2] = (size>>8)&0xFF;
            this->bin[pos+3] = size&0xFF;
        }
        
        void setString(std::string str, unsigned int length=4) {
            unsigned char *s = (unsigned char *)str.c_str();
            for(s64 n=0; n<str.length(); n++) this->setU8(s[n]);
            s64 len = length-str.length();
            if(len>=1) {
                while(len--) this->setU8(0);
            }
        }
        
        void setPascalString(std::string str) {
            //assert(str.length()<255);
            this->setU8(str.size());
            this->setString(str,(unsigned int)str.size());
        }
        
        void setCompressorName(std::string str) {
            //assert(str.length()<32);
            this->setPascalString(str);
            int len = 31-(int)str.length();
            while(len--) this->setU8(0);
        }
        
        void setMatrix() {
            // All values in the matrix are 32-bit fixed-point numbers divided as 16.16, except for the {u, v, w} column, which contains 32-bit fixed-point numbers divided as 2.30.
            this->setU32(0x00010000);
            this->setU32(0x00000000);
            this->setU32(0x00000000);
            this->setU32(0x00000000);
            this->setU32(0x00010000);
            this->setU32(0x00000000);
            this->setU32(0x00000000);
            this->setU32(0x00000000);
            this->setU32(0x40000000);
        }
        
        void setVersionWithFlag(unsigned char version=0, unsigned int flag=0) {
            this->setU8(version);
            this->setU8((flag>>16)&0xFF);
            this->setU8((flag>>8)&0xFF);
            this->setU8(flag&0xFF);
        }
        
    public:
        
        std::vector<unsigned char> *get() {
            return &this->bin;
        }
        
        moov(std::vector<TrackInfo> *info, SampleData *mdat, unsigned char *sps, u64 sps_size, unsigned char *pps, u64 pps_size, unsigned char *vps=nullptr, u64 vps_size=0) {
            
            this->reset();
            
            Atom moov = this->initAtom("moov");
            
            unsigned int maxDuration = 0;
            for(int n=0; n<info->size(); n++) {
                unsigned int TotalFrames = (unsigned int)(mdat->keyframes(n)->size());
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
                bool hvc1 = (((*info)[n].type)=="hvc1")?true:false;
                
                unsigned int track = n+1;
                unsigned int TotalFrames = (unsigned int)(mdat->keyframes(n)->size());
                
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
                
                if(avc1) {
                    this->setCompressorName("H.264"); // 32
                }
                else if(hvc1) {
                    this->setCompressorName("HEVC"); // 32
                }
                else {
                    this->setCompressorName("'"+((*info)[n].type)+"'"); // 32
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
                    
                    this->setU16(swapU32(*((unsigned int *)sps))&0xFFFF);
                    unsigned char *bytes = ((unsigned char *)sps)+4;
                    for(u64 n=0; n<sps_size-4; n++) {
                        this->bin.push_back(bytes[n]);
                    }
                    this->setU8(1); // 1
                    this->setU16(swapU32(*((unsigned int *)pps))&0xFFFF);
                    bytes = ((unsigned char *)pps)+4;
                    for(u64 n=0; n<pps_size-4; n++) {
                        this->bin.push_back(bytes[n]);
                    }
                    this->setAtomSize(avcC.second);
                }
                else if(hvc1&&vps&&sps&&pps) {
                    
                    Atom hvcC = this->initAtom("hvcC");
                    
                    this->setU8(1); // configurationVersion
                    
                    this->setU8(1); // general_profile_space, general_tier_flag, general_profile_idc
                    this->setU32(0b01100000000000000000000000000000); // general_profile_compatibility
                    
                    // general_constraint_indicator
                    this->setU8(1<<7|0<<6|1<<5|1<<4); // general_progressive_source_flag, general interlaced source flag, general_non packed_constraint_flag, general_frame_only_constraint_flag
                    this->setU8(0);
                    this->setU32(0);
                    
                    this->setU8(6.1*30); // general_level_idc [1,2,2.1,3,3.1,4,4.1,5,5.1,5.2,6,6.1,6.2]*30.0
                    this->setU16(0xF000); // min_spatial_segmentation_idc
                    this->setU8(0xFC); // parallelismType
                    this->setU8(0xFD); // chroma_format_idc (4:2:0)
                    this->setU8(0xF8); // bit_depth_luma_minus8
                    this->setU8(0xF8); // bit_depth_chroma_minus8
                    this->setU16(0); // avgFrameRate
                    this->setU8(1<<3|3); // numTemporalLayers, temporalIdNested, lengthSizeMinusOne
                    
                    this->setU8(3); // numOfArrays
                    
                    this->setU8(1<<7|HEVCNaluType::VPS);
                    this->setU16(1);
                    this->setU16(swapU32(*((unsigned int *)vps))&0xFFFF);
                    unsigned char *bytes = ((unsigned char *)vps)+4;
                    for(u64 n=0; n<vps_size-4; n++) {
                        this->bin.push_back(bytes[n]);
                    }
                    
                    this->setU8(1<<7|HEVCNaluType::SPS);
                    this->setU16(1);
                    this->setU16(swapU32(*((unsigned int *)sps))&0xFFFF);
                    bytes = ((unsigned char *)sps)+4;
                    for(u64 n=0; n<sps_size-4; n++) {
                        this->bin.push_back(bytes[n]);
                    }
                    
                    this->setU8(1<<7|HEVCNaluType::PPS);
                    this->setU16(1);
                    this->setU16(swapU32(*((unsigned int *)pps))&0xFFFF);
                    bytes = ((unsigned char *)pps)+4;
                    for(u64 n=0; n<pps_size-4; n++) {
                        this->bin.push_back(bytes[n]);
                    }
                    
                    this->setAtomSize(hvcC.second);
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
                
                if(!(avc1||hvc1))  {
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
                
                if(avc1||hvc1) {
                    
                    std::vector<bool> *keyframes = mdat->keyframes(n);
                    unsigned int TotalKeyframes = 0;
                    for(int k=0; k<TotalFrames; k++) {
                        if((*keyframes)[k]) TotalKeyframes++;
                    }
                    
                    this->initAtom("stss",8+4+4+TotalKeyframes*4);
                    this->setVersionWithFlag();
                    this->setU32(TotalKeyframes);
                    
                    for(int k=0; k<TotalFrames; k++) {
                        if((*keyframes)[k]) {
                            this->setU32(k+1); // 1 origin
                        }
                    }
                    
                    this->initAtom("sdtp",8+4+TotalFrames);
                    this->setVersionWithFlag();
                    
                    for(int k=0; k<TotalFrames; k++) {
                        this->setU8(((*keyframes)[k])?32:16);
                    }
                }
                
                Atom stsc = this->initAtom("stsc",28);
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                
                std::vector<unsigned int> *chunks = mdat->chunks(n);
                
                for(int k=0; k<chunks->size(); k++) {
                    this->setU32((*chunks)[k]); // First chunk
                    this->setU32(1); // Samples per chunk
                    this->setU32(1+k); // Sample description ID
                }
                
                this->setAtomSize(stsc.second);
                
                
                Atom stsz = this->initAtom("stsz",20);
                this->setVersionWithFlag();
                this->setU32(0); // If this field is set to 0, then the samples have different sizes, and those sizes are stored in the sample size table.
                this->setU32(TotalFrames); // Number of entries
                
                this->setAtomSize(stsz.second);
                
                std::vector<unsigned int> *lengths = mdat->lengths(n);
                for(int k=0; k<lengths->size(); k++) {
                    this->setU32((*lengths)[k]);
                }
                
                this->setAtomSize(stsz.second);
                
                if(this->is64) {
                    Atom co64 = this->initAtom("co64");
                    this->setVersionWithFlag();
                    this->setU32(TotalFrames);
                    std::vector<unsigned long> *offsets = mdat->offsets(n);
                    for(int k=0; k<offsets->size(); k++) {
                        this->setU64((*offsets)[k]);
                    }
                    this->setAtomSize(co64.second);
                }
                else {
                    Atom stco = this->initAtom("stco");
                    this->setVersionWithFlag();
                    this->setU32(TotalFrames);
                    std::vector<unsigned long> *offsets = mdat->offsets(n);
                    for(int k=0; k<offsets->size(); k++) {
                        this->setU32((unsigned int)((*offsets)[k]));
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
        
        ~moov() {
            this->reset();
        }
    };
}

namespace MultiTrackQTMovie {

    class Buffer {
        
        private:
            
            bool _keyframe = false;
            unsigned char *_bytes = nullptr;
            unsigned int _length = 0;
        
        public:
        
            Buffer(unsigned char *bytes, unsigned int length, bool keyframe) {
                this->_bytes = new unsigned char[length];
                this->_keyframe = keyframe;
                this->_length = length;
                memcpy(this->_bytes,bytes,this->_length);
            }
        
            bool keyframe() { return this->_keyframe; }
            unsigned int length() { return this->_length; }
            unsigned char *bytes() { return this->_bytes; }
        
            ~Buffer() {
                
                if(this->_bytes) {
                    delete[] this->_bytes;
                    this->_bytes = nullptr;
                }
            }
    };
    
    class VideoRecorder {
        
        protected:
            
            NSString *_path;
                    
            bool _isRunning = false;
            bool _isSaving = false;
            
            NSFileHandle *_handle;
            std::vector<u64> *_frames;
                        
            std::vector<TrackInfo> *_info;
            
            NSData *_vps = nil;
            NSData *_sps = nil;
            NSData *_pps = nil;
        
            NSString *filename() {
                long unixtime = (CFAbsoluteTimeGetCurrent()+kCFAbsoluteTimeIntervalSince1970)*1000;
                NSString *timeStampString = [NSString stringWithFormat:@"%f",(unixtime/1000.0)];
                NSDate *date = [NSDate dateWithTimeIntervalSince1970:[timeStampString doubleValue]];
                NSDateFormatter *format = [[NSDateFormatter alloc] init];
                [format setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"ja_JP"]];
                [format setDateFormat:@"yyyy_MMdd_HHmm_ss_SSS"];
    #if TARGET_OS_OSX
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSMoviesDirectory,NSUserDomainMask,YES);
    #else
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES);
    #endif
                NSString *directory = [paths objectAtIndex:0];
                return [NSString stringWithFormat:@"%@/%@",directory,[format stringFromDate:date]];
            }
            
        public:
            
            VideoRecorder(NSString *fileName, std::vector<TrackInfo> *info) {
                
                if(fileName) this->_path = fileName;
                else this->_path = [NSString stringWithFormat:@"%@.mov",this->filename()];
                this->_info = info;
            }
        
            NSString *path() { return this->_path; }
        
            bool isVPS() { return this->_vps?true:false; }
            void setVPS(NSData *data) { this->_vps = [[NSData alloc] initWithData:data]; }
            
            bool isSPS() { return this->_sps?true:false; }
            void setSPS(NSData *data) { this->_sps = [[NSData alloc] initWithData:data]; }
            
            bool isPPS() { return this->_pps?true:false; }
            void setPPS(NSData *data) { this->_pps = [[NSData alloc] initWithData:data]; }
        
            ~VideoRecorder() {
            }
    };


    class Recorder : public VideoRecorder {
        
        protected:
                
            dispatch_source_t _timer = nullptr;
            std::vector<Buffer *> *_queue;
            SampleData * _mdat = nullptr;
                    
            void inialized() {
                
                if(this->_info->size()>=1) {
                    this->_queue = new std::vector<Buffer *>[this->_info->size()];
                }
                
                MultiTrackQTMovie::ftyp *ftyp = new MultiTrackQTMovie::ftyp();
                
                unsigned long length = 0;
                
                std::vector<unsigned char> *bin = ftyp->get();
                [[[NSData alloc] initWithBytes:bin->data() length:bin->size()] writeToFile:this->_path options:NSDataWritingAtomic error:nil];
                length = bin->size();
                
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_path];
                [this->_handle seekToEndOfFile];
                
                this->_mdat = new SampleData(this->_handle,length,(unsigned int)this->_info->size());
            }
        
            void cleanup() {
                if(this->_timer){
                    dispatch_source_cancel(this->_timer);
                    this->_timer = nullptr;
                }
            }
        
            void setup() {
                this->_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("ENTER_FRAME",0));
                dispatch_source_set_timer(this->_timer,DISPATCH_TIME_NOW,NSEC_PER_SEC/300.0,0);
                dispatch_source_set_event_handler(this->_timer,^{
                    
                    if(!(this->_isSaving||this->_isRunning)) return;
                    
                    const unsigned long trackNum = this->_info->size();
                    
                    bool finished[trackNum];
                    for(int n=0; n<trackNum; n++) {
                        
                        unsigned long size = this->_queue[n].size();
                        
                        finished[n] = (size==0)?true:false;
                        
                        for(int k=0; k<size; k++) {

                            if(this->_queue[n][k]) {
                                 
                                bool keyframe = this->_queue[n][k]->keyframe();
                                unsigned int length = this->_queue[n][k]->length();
                                unsigned char *bytes = this->_queue[n][k]->bytes();
                                
                                this->_mdat->writeData(this->_handle,bytes,length,keyframe,n);
                                
                                delete this->_queue[n][k];
                                this->_queue[n][k] = nullptr;
                                
                                break;
                            }
                            
                            if(k==size-1) finished[n] = true;
                        }
                    }
                    
                    int finishedTracks = 0;
                    for(int n=0; n<this->_info->size(); n++) {
                        finishedTracks+=(finished[n])?1:0;
                    }
                    
                    if(finishedTracks==trackNum&&this->_isSaving==true&&this->_isRunning==false) {
                        
                        if(this->_timer){
                            dispatch_source_cancel(this->_timer);
                            this->_timer = nullptr;
                        }
                        
                        bool avc1 = false;
                        bool hvc1 = false;
                        
                        this->_mdat->writeSize(this->_handle);

                        for(int n=0; n<this->_info->size(); n++) {
                            
                            
                            if((*this->_info)[n].type=="avc1") {
                                avc1 = true;
                            }
                            else if((*this->_info)[n].type=="hvc1") {
                                hvc1 = true;
                            }
                            
                            if(hvc1) {
                                if(!(this->_vps&&this->_sps&&this->_pps)) {
                                    this->_isRunning = false;
                                    this->_isSaving = true;
                                    return;
                                }
                            }
                            else if(avc1) {
                                if(!(this->_sps&&this->_pps)) {
                                    this->_isRunning = false;
                                    this->_isSaving = true;
                                    return;
                                }
                            }
                        }
                        
                        MultiTrackQTMovie::moov *moov = nullptr;
                        
                        if(hvc1) {
                            moov = new MultiTrackQTMovie::moov(this->_info,this->_mdat,(unsigned char *)[this->_sps bytes],[this->_sps length],(unsigned char *)[this->_pps bytes],[this->_pps length],(unsigned char *)[this->_vps bytes],[this->_vps length]);
                        }
                        else if(avc1) {
                            moov = new MultiTrackQTMovie::moov(this->_info,this->_mdat,(unsigned char *)[this->_sps bytes],[this->_sps length],(unsigned char *)[this->_pps bytes],[this->_pps length],nullptr,0);
                        }
                        else {
                            moov = new MultiTrackQTMovie::moov(this->_info,this->_mdat,nullptr,0,nullptr,0,nullptr,0);
                        }
                      
                        [this->_handle seekToEndOfFile];
                        
                        std::vector<unsigned char> *bin = moov->get();
                        [this->_handle writeData:[[NSData alloc] initWithBytes:bin->data() length:bin->size()]];
                        delete moov;
                        
                        this->_isSaving = false;
    
                        EventEmitter::emit(MultiTrackQTMovie::Event::SAVE_COMPLETE);
                        
                    }
                });
                if(this->_timer) dispatch_resume(this->_timer);
            }
        
        public:
            
            Recorder(NSString *fileName,std::vector<TrackInfo> *info) : VideoRecorder(fileName,info) {
                this->inialized();
                this->setup();
            }
            
            Recorder(std::vector<TrackInfo> *info) : VideoRecorder(nil,info) {
                this->inialized();
                this->setup();
            }
            
            Recorder *add(unsigned char *data, unsigned int length, unsigned int trackid, bool keyframe=true) {
                if(!this->_isSaving) {
                    if(this->_isRunning==false) this->_isRunning = true;
                    if(trackid>=0&&trackid<this->_info->size()) {
                        this->_queue[trackid].push_back(new Buffer(data,length,keyframe));
                    }
                }
                return this;
            }
            
            void save() {                
                if(this->_isRunning&&!this->_isSaving) {
                    this->_isSaving = true;
                    this->_isRunning = false;
                }
            }
            
            ~Recorder() {
                
            }
    };
};
