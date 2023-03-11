#pragma once
#import <vector>

namespace EventEmitter {

    class Observer {
        
        private:
            
            NSString *_type = nil;
            id _observer = nil;
        
        public:
            
            Observer(NSString *type, id observer) {
                this->_type = type;
                this->_observer = observer;
            }
        
            NSString *type() { return this->_type; }
            id observer() { return this->_observer; }
        
            ~Observer() {
                this->_observer = nil;
            }
    };
    
    std::vector<Observer *> events;

    void on(NSString *type, void (^callback)(NSNotification *)) {
                
        id observer = nil;
        
        long len = events.size();
        while(len--) {
            if(events[len]->type()&&[events[len]->type() compare:type]==NSOrderedSame) {
                observer = events[len]->observer();
                break;
            }
        }
        
        if(observer==nil) {
            
            observer = [[NSNotificationCenter defaultCenter]
                addObserverForName:type
                object:nil
                queue:[NSOperationQueue mainQueue]
                usingBlock:callback
            ];
            
            events.push_back(new Observer(type,observer));
        }
        else {
            NSLog(@"%@ is already registered",type);
        }
    }

    void off(NSString *type) {
        
        id observer = nil;
        
        long len = events.size();
        while(len--) {
            if(events[len]->type()&&[events[len]->type() compare:type]==NSOrderedSame) {
                observer = events[len]->observer();
                events.erase(events.begin()+len);
                break;
            }
        }
        
        if(observer) {
            [[NSNotificationCenter defaultCenter] removeObserver:(id)observer];
            observer = nil;
        }
    }

    void emit(NSString *event) {
        [[NSNotificationCenter defaultCenter] postNotificationName:event object:nil];
    }
};
