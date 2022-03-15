#import "GuideLayer.h"

class Guide {
    
    private:

        NSView *_view = nil;
        GuideLayer<Plane> *_layer = nullptr;
        unsigned int *_texture = nullptr;
    
        int _frame = 0;

        float _pattern[64] = {
            0,0,0,0,1,1,1,1,
            0,0,0,1,1,1,1,0,
            0,0,1,1,1,1,0,0,
            0,1,1,1,1,0,0,0,
            1,1,1,1,0,0,0,0,
            1,1,1,0,0,0,0,1,
            1,1,0,0,0,0,1,1,
            1,0,0,0,0,1,1,1
        };
    
    public:

        NSView *view() {
            return this->_view;
        }
    
        void clear() {
            for(int k=0; k<STAGE_WIDTH*STAGE_HEIGHT; k++) {
                this->_texture[k] = 0;
            }
        }
    
        void update(int left,int top, int right, int bottom) {
            
            int cnt = (this->_frame++)>>1;
            if(this->_frame==8) this->_frame = 0;
            
            for(int k=left; k<right; k++) {
                int x = (k+cnt)%8;
                this->_texture[top*STAGE_WIDTH+k] = this->_pattern[((top+cnt)%8)*8+x]?WHITE:BLACK;
                this->_texture[bottom*STAGE_WIDTH+k] = this->_pattern[((bottom+cnt)%8)*8+x]?WHITE:BLACK;
            }
            
            for(int k=top; k<bottom; k++) {
                int y = (k+cnt)%8;
                this->_texture[k*STAGE_WIDTH+left] = this->_pattern[y*8+((left+cnt)%8)]?WHITE:BLACK;
                this->_texture[k*STAGE_WIDTH+right] = this->_pattern[y*8+((right+cnt)%8)]?WHITE:BLACK;
            }
            
        }
        
        void draw() {
            
            [this->_layer->texture() replaceRegion:MTLRegionMake2D(0,0,this->_layer->width(),this->_layer->height()) mipmapLevel:0 withBytes:this->_texture bytesPerRow:this->_layer->width()<<2];
            
            this->_layer->update(^(id<MTLCommandBuffer> commandBuffer){
                this->_layer->cleanup();
            });
                
        }
    
        Guide() {
            
            this->_texture = new unsigned int[STAGE_WIDTH*STAGE_HEIGHT];
            
            this->_view = (NSView *)[[NSView alloc] initWithFrame:CGRectMake(0,0,STAGE_WIDTH,STAGE_HEIGHT)];
            this->_view.wantsLayer = YES;
            this->_view.layer.backgroundColor = [NSColor clearColor].CGColor;
            
            this->_layer = new GuideLayer<Plane>();
            if(this->_layer->init(STAGE_WIDTH,STAGE_HEIGHT,@"guide.metallib",[[NSBundle mainBundle] bundleIdentifier])) {
                this->_view.layer = this->_layer->layer();
            }
        }
        
        ~Guide() {
            [this->_view removeFromSuperview];
        }
    
};
