#import <vector>

#ifdef EMSCRIPTEN
    typedef unsigned long long u64;
    typedef long long s64;
#else
    typedef unsigned long u64;
    typedef long s64;
#endif

namespace MultiTrackQTMovie {

    typedef struct _TrackInfo {
        unsigned short width = 1920;
        unsigned short height = 1080;
        unsigned short depth = 24.0;
        double fps = 30.0;
        bool encode = false;
        std::string codec = "hvc1";
        std::string trak = "trak";
    } TrackInfo;

#ifdef EMSCRIPTEN
    
    u64 toU64(unsigned char *p) {
        U64 p0 = p[0];
        U64 p1 = p[1];
        U64 p2 = p[2];
        U64 p3 = p[3];
        U64 p4 = p[4];
        U64 p5 = p[5];
        U64 p6 = p[6];
        U64 p7 = p[7];
        return p7<<56|p6<<48|p5<<40|p4<<32|p3<<24|p2<<16|p1<<8|p0;
    }
    
    unsigned int toU32(unsigned char *p) {
        return p[3]<<24|p[2]<<16|p[1]<<8|p[0];
    }
    
    unsigned short toU16(unsigned char *p) {
        return p[1]<<8|p[0];
    }
    
    u64 swapU64(u64 n) {
        U64 p0 = (n>>56)&0xFF;
        U64 p1 = (n>>48)&0xFF;
        U64 p2 = (n>>40)&0xFF;
        U64 p3 = (n>>32)&0xFF;
        U64 p4 = (n>>24)&0xFF;
        U64 p5 = (n>>16)&0xFF;
        U64 p6 = (n>>8)&0xFF;
        U64 p7 = (n)&0xFF;
        return p7<<56|p6<<48|p5<<40|p4<<32|p3<<24|p2<<16|p1<<8|p0;
    }
    
#else
    
    u64 toU64(unsigned char *p) {
        return *((u64 *)p);
    }
    
    unsigned int toU32(unsigned char *p) {
        return *((unsigned int *)p);
    }
    
    unsigned short toU16(unsigned char *p) {
        return *((unsigned short *)p);
    }
    
    u64 swapU64(u64 n) {
        return ((n>>56)&0xFF)|(((n>>48)&0xFF)<<8)|(((n>>40)&0xFF)<<16)|(((n>>32)&0xFF)<<24)|(((n>>24)&0xFF)<<32)|(((n>>16)&0xFF)<<40)|(((n>>8)&0xFF)<<48)|((n&0xFF)<<56);
    }
    
#endif
    
    unsigned int swapU32(unsigned int n) {
        return ((n>>24)&0xFF)|(((n>>16)&0xFF)<<8)|(((n>>8)&0xFF)<<16)|((n&0xFF)<<24);
    }
    
    unsigned short swapU16(unsigned short n) {
        return ((n>>8)&0xFF)|((n&0xFF)<<8);
    }
    
    
    u64 U64(unsigned char *p) {
        return swapU64(toU64(p));
    }
    
    unsigned int U32(unsigned char *p) {
        return swapU32(toU32(p));
    }
    
    unsigned short U16(unsigned char *p) {
        return swapU16(toU16(p));
    }
}

namespace AVCNaluType {
    const unsigned char SPS = 7;
    const unsigned char PPS = 8;
}

namespace HEVCNaluType {
    const unsigned char VPS = 32;
    const unsigned char SPS = 33;
    const unsigned char PPS = 34;
}

namespace MultiTrackQTMovie {
    
    class AtomUtils {
        
    protected:
        
        void reset() {
            this->bin.clear();
        }
        
        std::vector<unsigned char> bin;
        unsigned int CreationTime = 3061152000;
        
        void setU64(u64 value) {
            bin.push_back((value>>56)&0xFF);
            bin.push_back((value>>48)&0xFF);
            bin.push_back((value>>40)&0xFF);
            bin.push_back((value>>32)&0xFF);
            bin.push_back((value>>24)&0xFF);
            bin.push_back((value>>16)&0xFF);
            bin.push_back((value>>8)&0xFF);
            bin.push_back((value)&0xFF);
        }
        
        void setU32(unsigned int value) {
            bin.push_back((value>>24)&0xFF);
            bin.push_back((value>>16)&0xFF);
            bin.push_back((value>>8)&0xFF);
            bin.push_back((value)&0xFF);
        }
        
        void setU16(unsigned short value) {
            bin.push_back((value>>8)&0xFF);
            bin.push_back((value)&0xFF);
        }
        
        void setU8(unsigned char value) {
            bin.push_back(value);
        }
        
        void setZero(u64 length) {
            for(u64 n=0; n<length; n++) bin.push_back(0);
        }
    };
}

namespace MultiTrackQTMovie {
    
    typedef std::pair<std::string,u64> Atom;
}
