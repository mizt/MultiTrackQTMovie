#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {

        MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(@"2022_06_17_19_20_17_164.mov");
        
        for(int n=0; n<parser->tracks(); n++) {
            
            NSString *fn = [NSString stringWithFormat:@"./track-%d.mov",n];
            
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
}