#import <vector>

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
