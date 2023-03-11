#import <vector>
#import <string>

#import "MultiTrackQTMovieEvent.h"
#import "MultiTrackQTMovieUtils.h"
#import "MultiTrackQTMovieSampleData.h"
#import "MultiTrackQTMovieAtomUtils.h"

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
            std::vector<SampleData *> _mdat;
        
            unsigned long _offset = 0;
            
            void inialized() {
                
                if(this->_info->size()>=1) {
                    this->_queue = new std::vector<Buffer *>[this->_info->size()];
                }
                
                for(int n=0; n<this->_info->size(); n++) {
                    this->_mdat.push_back(nullptr);
                }
                
                MultiTrackQTMovie::ftyp *ftyp = new MultiTrackQTMovie::ftyp();
                
                unsigned long length = 0;
                
        #ifdef USE_VECTOR
                std::vector<unsigned char> *bin = ftyp->get();
                [[[NSData alloc] initWithBytes:bin->data() length:bin->size()] writeToFile:this->_path options:NSDataWritingAtomic error:nil];
                length = bin->size();
        #else
                NSData *bin = ftyp->get();
                [bin writeToFile:this->_path options:NSDataWritingAtomic error:nil];
                length = [bin length];
        #endif
                
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_path];
                [this->_handle seekToEndOfFile];
                
                this->_offset = length;
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
                    for(int n=0; n<trackNum; n++) finished[n] = false;
                    
                    for(int n=0; n<trackNum; n++) {
                        
                        unsigned long size = this->_queue[n].size();
                        for(int k=0; k<size; k++) {

                            if(this->_queue[n][k]) {
                                 
                                bool keyframe = this->_queue[n][k]->keyframe();
                                unsigned int length = this->_queue[n][k]->length();
                                unsigned char *bytes = this->_queue[n][k]->bytes();
                                
                                if(this->_mdat[n]==nullptr) {
                                    this->_mdat[n] = new SampleData(this->_handle,this->_offset);
                                }
                                
                                this->_mdat[n]->writeData(this->_handle,bytes,length,keyframe);
                                
                                delete this->_queue[n][k];
                                this->_queue[n][k] = nullptr;
                                
                                NSLog(@"%d",k);
                                
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
                        
                        for(int n=0; n<this->_info->size(); n++) {
                            
                            this->_mdat[n]->writeSize(this->_handle);
                            
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
                        
    #ifdef USE_VECTOR
                        std::vector<unsigned char> *bin = moov->get();
                        [this->_handle writeData:[[NSData alloc] initWithBytes:bin->data() length:bin->size()]];

    #else
                        [this->_handle writeData:moov->get()];

    #endif
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

        
        
        
        