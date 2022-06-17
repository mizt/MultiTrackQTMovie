#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {
        MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(@"24fps.mov");
        NSLog(@"tracks = %d",parser->tracks());
        
        for(int n=0; n<parser->tracks(); n++) {
            NSLog(@"%s",parser->type(n).c_str());
            NSLog(@"FPS = %d",parser->FPS(n));
            NSLog(@"length = %d",parser->length(n));
            NSLog(@"width = %d",parser->width(n));
            NSLog(@"height = %d",parser->height(n));
        }
    }
}