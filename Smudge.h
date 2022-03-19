class Smudge : public MapRG16UnormBase {
    
    public:
    
        void resetMap() {
            for(int i=0; i<this->height; i++) {
                for(int j=0; j<this->width; j++) {
                    this->_map[0][i*this->width+j] = (0x7FFF+j*4)<<16|(0x7FFF+i*4);
                    this->_map[1][i*this->width+j] = (0x7FFF+j*4)<<16|(0x7FFF+i*4);
                }
            }
        }
        
        void reset(float mx,float my) {
            float *prevCursor = (float *)[this->_params[0] contents];
            float *cursor = (float *)[this->_params[1] contents];
            
            prevCursor[0] = cursor[0] = mx;
            prevCursor[1] = cursor[1] = my;
        }
    
        unsigned int *map(float mx,float my) {
            
            float *prevCursor = (float *)[this->_params[0] contents];
            float *cursor = (float *)[this->_params[1] contents];
            
            prevCursor[0] = cursor[0];
            prevCursor[1] = cursor[1];
                
            cursor[0] = mx;
            cursor[1] = my;
              
            return this->exec();
        }
    
        Smudge(int w,int h,NSString *path) : MapRG16UnormBase(w, h, path) {
        
            this->_params.push_back([this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault]); // prevCursor
            this->_params.push_back([this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault]); // cursor
        }
            
        ~Smudge() {
            
        }
};
