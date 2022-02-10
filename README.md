Read/Write MultiTrack QuickTime PNG/JPEG sequences.

### read

```
#import <Foundation/Foundation.h>
#import "MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {
        MultiTrackQTMovie::Parser *parser = new MultiTrackQTMovie::Parser(@"test.mov");
        NSLog(@"tracks = %d",parser->tracks());
        
        for(int n=0; n<parser->tracks(); n++) {
            NSLog(@"%s",parser->type(n).c_str());
            NSLog(@"length = %d",parser->length(n));
            NSLog(@"width = %d",parser->width(n));
            NSLog(@"height = %d",parser->height(n));
        }
        
        if(parser->type(0)=="jpeg") {
            [parser->get(0,0) writeToFile:@"./track-0.jpg" options:NSDataWritingAtomic error:nil];
        }
        if(parser->type(1)=="png ") {
            [parser->get(0,1) writeToFile:@"./track-1.png" options:NSDataWritingAtomic error:nil];
        }
    }
}
```

### write

```
#import <Foundation/Foundation.h>
#import "MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
    @autoreleasepool {
        NSData *image = [[NSFileManager defaultManager] contentsAtPath:@"track-0.jpg"];
        NSData *data = [[NSFileManager defaultManager] contentsAtPath:@"track-1.png"];
        
        std::vector<MultiTrackQTMovie::TrackInfo> info;
        info.push_back({.width=1440,.height=1920,.depth=24,.fps=30.,.type="jpeg"});
        info.push_back({.width=256*4,.height=(192+1),.depth=32,.fps=30.,.type="png "});
        
        MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(@"test.mov",&info);
        for(int k=0; k<30; k++) {
            recorder->add((unsigned char *)[image bytes],(unsigned int)[image length],0);
            recorder->add((unsigned char *)[data bytes],(unsigned int)[data length],1);
        }
        recorder->save();
    }
}
```

##### Note: iOS

Need to add two key in info.plist with YES.

- [`UIFileSharingEnabled`](https://developer.apple.com/library/content/documentation/General/Reference/InfoPlistKeyReference/Articles/iPhoneOSKeys.html#//apple_ref/doc/uid/TP40009252-SW20) (`Application supports iTunes file sharing`)
- [`LSSupportsOpeningDocumentsInPlace`](https://developer.apple.com/library/content/documentation/General/Reference/InfoPlistKeyReference/Articles/LaunchServicesKeys.html#//apple_ref/doc/uid/TP40009250-SW13) (`Supports opening documents in place`)
