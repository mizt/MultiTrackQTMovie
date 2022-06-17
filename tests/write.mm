#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
	@autoreleasepool {
		NSData *image = [[NSFileManager defaultManager] contentsAtPath:@"test.jpg"];
		
		std::vector<MultiTrackQTMovie::TrackInfo> info;
		info.push_back({.width=1920,.height=1080,.depth=24,.fps=24.,.type="jpeg"});
		
		MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(@"24fps.mov",&info);
		for(int k=0; k<24; k++) {
			recorder->add((unsigned char *)[image bytes],(unsigned int)[image length],0);
		}
		recorder->save();
	}
}