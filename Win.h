class Win {
    
    private:
    
        NSWindow *win = nil;
        
    public:
    
        void addChild(NSView *view) {
            [[this->win contentView] addSubview:view];
        }
    
        Win() {
            this->win = [[NSWindow alloc] initWithContentRect:CGRectMake(0,0,STAGE_WIDTH,STAGE_HEIGHT) styleMask:1|1<<2 backing:NSBackingStoreBuffered defer:NO];
            this->win.backgroundColor =  [NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1];
            
            CGSize screenSize = [[NSScreen mainScreen] frame].size;
            CGSize windowSize = this->win.frame.size;

            [this->win setFrame:CGRectMake((screenSize.width -windowSize.width)*0.5,(screenSize.height-windowSize.height)*0.5, windowSize.width,windowSize.height) display:YES];
            [this->win makeKeyAndOrderFront:nil];
        }
       
        ~Win() {
            [this->win setReleasedWhenClosed:NO];
            [this->win close];
            this->win = nil;
        }
    
};