#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovieParser.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {
        
        if(argc==2) {
            NSString *SRC = [NSString stringWithFormat:@"%s",argv[1]];
            NSLog(@"%@",SRC);
            MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(SRC);
            
            for(int n=0; n<parser->tracks(); n++) {
                
                NSString *fn = [NSString stringWithFormat:@"%@%@.mov",[SRC stringByDeletingPathExtension],(parser->type(n)=="png ")?@"-pointcloud":@"-texture"];
                
                unsigned short w = parser->width(n);
                unsigned short h = parser->height(n);
                unsigned short depth = parser->depth(n);
                double FPS = parser->FPS(n);
                
                std::vector<MultiTrackQTMovie::TrackInfo> info;
                info.push_back({.width=w,.height=h,.depth=depth,.fps=FPS,.type=parser->type(n)});
                
                MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(fn,&info);
                
                int len = parser->length(n);
                for(int k=0; k<len; k++) {
                    NSData *image = parser->get(k,n);
                    recorder->add((unsigned char *)[image bytes],(unsigned int)[image length],0);
                }
                recorder->save();
                
            }

        }
        else {
            NSLog(@"Err: Require a path as argument");
        }
        
    }
}