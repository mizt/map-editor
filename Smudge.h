class Smudge : public MapRG16UnormBase {
    
    public:
    
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
