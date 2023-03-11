#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
	@autoreleasepool {
		
		__block bool wait = true;

		NSData *image = [[NSFileManager defaultManager] contentsAtPath:@"./test.jpg"];
		
		std::vector<MultiTrackQTMovie::TrackInfo> info;
		info.push_back({.width=1920,.height=1080,.depth=24,.fps=30.,.type="jpeg"});
		
		MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(@"./test.mov",&info);
						
		EventEmitter::on(MultiTrackQTMovie::Event::SAVE_COMPLETE,^(NSNotification *notification) {
			EventEmitter::off(MultiTrackQTMovie::Event::SAVE_COMPLETE);
			
			delete recorder;
			
			wait = false;
		});
		
		for(int k=0; k<30; k++) {
			recorder->add((unsigned char *)[image bytes],(unsigned int)[image length],0);
		}
		
		recorder->save();
		
		while(wait) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0f]];
		}
	}
}