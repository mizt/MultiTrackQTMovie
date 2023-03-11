#import <Foundation/Foundation.h>
#import "../MultiTrackQTMovie.h"

int main(int argc, char *argv[]) {
	@autoreleasepool {
		
		__block bool wait = true;

		std::vector<MultiTrackQTMovie::TrackInfo> info;
		info.push_back({.width=1920,.height=1080,.depth=24,.fps=30.,.type="jpeg"});
		info.push_back({.width=1920,.height=1080,.depth=24,.fps=30.,.type="png "});
		
		MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(@"./test.mov",&info);
						
		EventEmitter::on(MultiTrackQTMovie::Event::SAVE_COMPLETE,^(NSNotification *notification) {
			EventEmitter::off(MultiTrackQTMovie::Event::SAVE_COMPLETE);
			delete recorder;
			wait = false;
		});
		
		for(int k=0; k<3; k++) {
			
			NSData *jpg = [[NSFileManager defaultManager] contentsAtPath:[NSString stringWithFormat:@"./%05d.jpg",k]];
			NSData *png = [[NSFileManager defaultManager] contentsAtPath:[NSString stringWithFormat:@"./%05d.png",k]];
			
			recorder->add((unsigned char *)[jpg bytes],(unsigned int)[jpg length],0,true);
			recorder->add((unsigned char *)[png bytes],(unsigned int)[png length],1,true);
		}
		
		recorder->save();
		
		while(wait) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0f]];
		}
	}
}