#import <vector>

namespace MultiTrackQTMovie {
        class SampleData {
            
        private:
            
            unsigned long _offset = 0;
            unsigned long _length = 0;
            
            std::vector<bool> _keyframes;
            std::vector<unsigned long> _offsets;
            std::vector<unsigned int> _lengths;
            
        public:
            
            unsigned long length() { return this->_length; }
            unsigned long offset() { return this->_offset; };
            
            std::vector<bool> *keyframes() { return &this->_keyframes; }
            std::vector<unsigned long> *offsets() { return &this->_offsets; }
            std::vector<unsigned int> *lengths() { return &this->_lengths; };
            
            void writeData(NSFileHandle *handle, unsigned char *bytes, unsigned int length, bool keyframe) {
                this->_offsets.push_back(this->_offset+this->_length);
                this->_lengths.push_back(length);
                [handle writeData:[[NSData alloc] initWithBytes:bytes length:length]];
                [handle seekToEndOfFile];
                this->_length+=length;
                this->_keyframes.push_back(keyframe);
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
            
            SampleData(NSFileHandle *handle, unsigned long offset) {
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
                this->_keyframes.clear();
                this->_offsets.clear();
                this->_lengths.clear();
            }
        };
}
